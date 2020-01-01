#include "KernelReload.h"


////////////////////////////////////////////
/////////////  内核重载相关函数   /////////////
////////////////////////////////////////////

// 导入SSDT全局变量
NTSYSAPI SSDTEntry	KeServiceDescriptorTable;


static char*		g_pNewNtKernel;		// 新内核
static ULONG		g_ntKernelSize;		// 内核的映像大小
static SSDTEntry*	g_pNewSSDTEntry;	// 新ssdt的入口地址	
static ULONG		g_hookAddr;			// 被hook位置的首地址
static ULONG		g_hookAddr_next_ins;// 被hook的指令的下一条指令的首地址.


// 读取NT内核模块
// 将读取到的缓冲区的内容保存到pBuff中.
NTSTATUS loadNtKernelModule( UNICODE_STRING* ntkernelPath, char** pBuff );

// 修复重定位.
void fixRelocation( char* pDosHdr, ULONG curNtKernelBase );

// 填充SSDT表
// char* pDos - 新加载的内核堆空间首地址
// char* pCurKernelBase - 当前正在使用的内核的加载基址
void initSSDT( char* pDos, char* pCurKernelBase );

// 安装HOOK
void installHook( );

// 卸载HOOK
void uninstallHook( );

// inline hook KiFastCallEntry的函数
 void myKiFastEntryHook( );


////////////////////////////////////////////
///////////////  驱动入口函数   //////////////
////////////////////////////////////////////
NTSTATUS OnKernelReload(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	pIrp;
	NTSTATUS status = STATUS_SUCCESS;
	DbgBreakPoint( );
	
	// 1. 找到内核文件路径
	// 1.1 通过遍历内核链表的方式来找到内核主模块
	MY_LDR_DATA_TABLE_ENTRY* pLdr = ((MY_LDR_DATA_TABLE_ENTRY*)pDevice->DriverObject->DriverSection);
	// 1.2 内核主模块在链表中的第2项.
	for(int i =0;i<2;++i)
		pLdr = (MY_LDR_DATA_TABLE_ENTRY*)pLdr->InLoadOrderLinks.Flink;
	
	g_ntKernelSize = pLdr->SizeOfImage;

	// 1.3 保存当前加载基址
	char* pCurKernelBase = (char*)pLdr->DllBase;
	
	KdPrint( ("base=%p name=%wZ\n", pCurKernelBase, &pLdr->FullDllName) );
	
	// 2. 读取nt模块的文件内容到堆空间.
	status = loadNtKernelModule( &pLdr->FullDllName, &g_pNewNtKernel );
	if(STATUS_SUCCESS != status)
	{
		return status;
	}

	// 3. 修复新nt模块的重定位.
	fixRelocation( g_pNewNtKernel, (ULONG)pCurKernelBase );

	// 4. 使用当前正在使用的内核的数据来填充
	//    新内核的SSDT表.
	initSSDT( g_pNewNtKernel, pCurKernelBase );

	// 5. HOOK KiFastCallEntry,使调用号走新内核的路线
	installHook( );

	pDevice->DriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}


VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	UNREFERENCED_PARAMETER(pDriver);
	uninstallHook( );
}



// 关闭内存页写入保护
void _declspec(naked) disablePageWriteProtect( )
{
	_asm
	{
		push eax;
		mov eax, cr0;
		and eax, ~0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}

// 开启内存页写入保护
void _declspec(naked) enablePageWriteProtect( )
{
	_asm
	{
		push eax;
		mov eax, cr0;
		or eax, 0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}


// 加载NT内核模块
// 将读取到的缓冲区的内容保存到pBuff中.
NTSTATUS loadNtKernelModule( UNICODE_STRING* ntkernelPath, char** pOut )
{
	NTSTATUS status = STATUS_SUCCESS;
	// 2. 获取文件中的内核模块
	// 2.1 将内核模块作为文件来打开.
	HANDLE hFile = NULL;
	char* pBuff = NULL;
	ULONG read = 0;
	char pKernelBuff[0x1000];

	status = createFile( ntkernelPath->Buffer,
						 GENERIC_READ,
						 FILE_SHARE_READ,
						 FILE_OPEN_IF,
						 FALSE,
						 &hFile );
	if(status != STATUS_SUCCESS)
	{
		KdPrint( ("打开文件失败\n") );
		goto _DONE;
	}

	// 2.2 将PE文件头部读取到内存
	status = readFile( hFile, 0, 0, 0x1000, pKernelBuff, &read );
	if(STATUS_SUCCESS != status)
	{
		KdPrint( ("读取文件内容失败\n") );
		goto _DONE;
	}

	// 3. 加载PE文件到内存.
	// 3.1 得到扩展头,获取映像大小. 
	IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pKernelBuff;
	IMAGE_NT_HEADERS* pnt = (IMAGE_NT_HEADERS*)((ULONG)pDos + pDos->e_lfanew);
	ULONG imgSize = pnt->OptionalHeader.SizeOfImage;
	
	// 3.2 申请内存以保存各个区段的内容.
	pBuff = ExAllocatePool( NonPagedPool, imgSize );
	if(pBuff == NULL)
	{
		KdPrint( ("内存申请失败失败\n") );
		status = STATUS_BUFFER_ALL_ZEROS;	// 随便返回个错误码
		goto _DONE;
	}

	// 3.2.1 拷贝头部到堆空间
	RtlCopyMemory( pBuff, 
				   pKernelBuff, 
				   pnt->OptionalHeader.SizeOfHeaders );

	// 3.3 得到区段头, 并将按照区段头将区段读取到内存中.
	IMAGE_SECTION_HEADER* pScnHdr = IMAGE_FIRST_SECTION( pnt );
	ULONG scnCount = pnt->FileHeader.NumberOfSections;
	for(ULONG i = 0; i < scnCount; ++i)
	{
		//
		// 3.3.1 读取文件内容到堆空间指定位置.
		//
		status = readFile( hFile,
						   pScnHdr[i].PointerToRawData,
						   0,
						   pScnHdr[i].SizeOfRawData,
						   pScnHdr[i].VirtualAddress + pBuff,
						   &read );
		if(status != STATUS_SUCCESS)
			goto _DONE;

	}


_DONE:
	ZwClose( hFile );
	//
	// 保存新内核的加载的首地址
	//
	*pOut = pBuff; 

	if(status != STATUS_SUCCESS )
	{
		if(pBuff != NULL)
		{
			ExFreePool( pBuff );
			*pOut = pBuff = NULL;
		}
	}
	return status;
}


// 修复重定位.
void fixRelocation( char* pDosHdr , ULONG curNtKernelBase )
{
	IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pDosHdr;
	IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)((ULONG)pDos + pDos->e_lfanew);
	ULONG uRelRva = pNt->OptionalHeader.DataDirectory[5].VirtualAddress;
	IMAGE_BASE_RELOCATION* pRel =
		(IMAGE_BASE_RELOCATION*)(uRelRva + (ULONG)pDos);

	while(pRel->SizeOfBlock)
	{
		typedef struct
		{
			USHORT offset : 12;
			USHORT type : 4;
		}TypeOffset;

		ULONG count = (pRel->SizeOfBlock - 8) / 2;
		TypeOffset* pTypeOffset = (TypeOffset*)(pRel + 1);
		for(ULONG i = 0; i < count; ++i)
		{
			if(pTypeOffset[i].type != 3)
			{
				continue;
			}

			ULONG* pFixAddr = (ULONG*)(pTypeOffset[i].offset + pRel->VirtualAddress + (ULONG)pDos);
			//
			// 减去默认加载基址
			//
			*pFixAddr -= pNt->OptionalHeader.ImageBase;
			
			//
			// 加上新的加载基址(使用的是当前内核的加载基址,这样做
			// 是为了让新内核使用当前内核的数据(全局变量,未初始化变量等数据).)
			//
			*pFixAddr += (ULONG)curNtKernelBase;
		}

		pRel = (IMAGE_BASE_RELOCATION*)((ULONG)pRel + pRel->SizeOfBlock);
	}
}

// 填充SSDT表
// char* pNewBase - 新加载的内核堆空间首地址
// char* pCurKernelBase - 当前正在使用的内核的加载基址
void initSSDT( char* pNewBase, char* pCurKernelBase )
{
	// 1. 分别获取当前内核,新加载的内核的`KeServiceDescriptorTable`
	//    的地址
	SSDTEntry* pCurSSDTEnt = &KeServiceDescriptorTable;
	g_pNewSSDTEntry = (SSDTEntry*)
		((ULONG)pCurSSDTEnt - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2. 获取新加载的内核以下三张表的地址:
	// 2.1 服务函数表基址
	g_pNewSSDTEntry->ServiceTableBase = (ULONG*)
		((ULONG)pCurSSDTEnt->ServiceTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.3 服务函数参数字节数表基址
	g_pNewSSDTEntry->ParamTableBase = (UCHAR*)
		((ULONG)pCurSSDTEnt->ParamTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.2 服务函数调用次数表基址(有时候这个表并不存在.)
	if(pCurSSDTEnt->ServiceCounterTableBase)
	{
		g_pNewSSDTEntry->ServiceCounterTableBase = (ULONG*)
			((ULONG)pCurSSDTEnt->ServiceCounterTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);
	}

	// 2.3 设置新SSDT表的服务个数
	g_pNewSSDTEntry->NumberOfServices = pCurSSDTEnt->NumberOfServices;

	//3. 将服务函数的地址填充到新SSDT表(重定位时其实已经修复好了,
	//   但是,在修复重定位的时候,是使用当前内核的加载基址的, 修复重定位
	//   为之后, 新内核的SSDT表保存的服务函数的地址都是当前内核的地址,
	//   在这里要将这些服务函数的地址改回新内核中的函数地址.)
	disablePageWriteProtect( );
	for(ULONG i = 0; i < g_pNewSSDTEntry->NumberOfServices; ++i)
	{
		// 减去当前内核的加载基址
		g_pNewSSDTEntry->ServiceTableBase[i] -= (ULONG)pCurKernelBase;
		// 换上新内核的加载基址.
		g_pNewSSDTEntry->ServiceTableBase[i] += (ULONG)pNewBase;
	}
	enablePageWriteProtect( );
}

void installHook( )
{
	g_hookAddr = 0;

	// 1. 找到KiFastCallEntry函数首地址
	ULONG uKiFastCallEntry = 0;
	_asm
	{
		;// KiFastCallEntry函数地址保存
		;// 在特殊模组寄存器的0x176号寄存器中
		push ecx;
		push eax;
		push edx;
		mov ecx, 0x176; // 设置编号
		rdmsr; ;// 读取到edx:eax
		mov uKiFastCallEntry, eax;// 保存到变量
		pop edx;
		pop eax;
		pop ecx;
	}

	// 2. 找到HOOK的位置, 并保存5字节的数据
	// 2.1 HOOK的位置选定为(此处正好5字节,):
	//  2be1            sub     esp, ecx ;
	// 	c1e902          shr     ecx, 2   ;
	UCHAR hookCode[5] = { 0x2b,0xe1,0xc1,0xe9,0x02 }; //保存inline hook覆盖的5字节
	ULONG i = 0;
	for(; i < 0x1FF; ++i)
	{
		if(RtlCompareMemory( (UCHAR*)uKiFastCallEntry + i,
							 hookCode,
							 5 ) == 5)
		{
			break;
		}
	}
	if(i >= 0x1FF)
	{
		KdPrint( ("在KiFastCallEntry函数中没有找到HOOK位置,可能KiFastCallEntry已经被HOOK过了\n") );
		uninstallHook( );
		return;
	}


	g_hookAddr = uKiFastCallEntry + i;
	g_hookAddr_next_ins = g_hookAddr + 5;

	// 3. 开始inline hook
	UCHAR jmpCode[5] = { 0xe9 };// jmp xxxx
	disablePageWriteProtect( );

	// 3.1 计算跳转偏移
	// 跳转偏移 = 目标地址 - 当前地址 - 5
	*(ULONG*)(jmpCode + 1) = (ULONG)myKiFastEntryHook - g_hookAddr - 5;

	// 3.2 将跳转指令写入
	RtlCopyMemory( (void*)(uKiFastCallEntry + i),
				   jmpCode,
				   5 );
	
	enablePageWriteProtect( );
}

void uninstallHook( )
{
	if(g_hookAddr)
	{
		
		// 将原始数据写回.
		UCHAR srcCode[5] = { 0x2b,0xe1,0xc1,0xe9,0x02 }; 
		disablePageWriteProtect( );

		// 3.1 计算跳转偏移
		// 跳转偏移 = 目标地址 - 当前地址 - 5

		_asm sti
		// 3.2 将跳转指令写入
		RtlCopyMemory( (void*)g_hookAddr,
					   srcCode,
					   5 );
		_asm cli
		g_hookAddr = 0;
		enablePageWriteProtect( );
	}

	if(g_pNewNtKernel)
	{
		ExFreePool( g_pNewNtKernel );
		g_pNewNtKernel = NULL;
	}
}


// SSDT过滤函数.
ULONG SSDTFilter( ULONG index,/*索引号,也是调用号*/
				  ULONG tableAddress,/*表的地址,可能是SSDT表的地址,也可能是Shadow SSDT表的地址*/
				  ULONG funAddr/*从表中取出的函数地址*/ )
{
	// 如果是SSDT表的话
	if(tableAddress == (ULONG)KeServiceDescriptorTable.ServiceTableBase)
	{
		// 判断调用号(190是ZwOpenProcess函数的调用号)
		if(index == 190)
		{
			// 返回新SSDT表的函数地址
			// 也就是新内核的函数地址.
			return g_pNewSSDTEntry->ServiceTableBase[190];
		}
	}
	// 返回旧的函数地址
	return funAddr;
}

// inline hook KiFastCallEntry的函数
void _declspec(naked) myKiFastEntryHook( )
{

	_asm
	{
		pushad; // 压栈寄存器: eax,ecx,edx,ebx, esp,ebp ,esi, edi
		pushfd; // 压栈标志寄存器

		push edx; // 从表中取出的函数地址
		push edi; // 表的地址
		push eax; // 调用号
		call SSDTFilter; // 调用过滤函数

		;// 函数调用完毕之后栈控件布局,指令pushad将
		;// 32位的通用寄存器保存在栈中,栈空间布局为:
		;// [esp + 00] <=> eflag
		;// [esp + 04] <=> edi
		;// [esp + 08] <=> esi
		;// [esp + 0C] <=> ebp
		;// [esp + 10] <=> esp
		;// [esp + 14] <=> ebx
		;// [esp + 18] <=> edx <<-- 使用函数返回值来修改这个位置
		;// [esp + 1C] <=> ecx
		;// [esp + 20] <=> eax
		mov dword ptr ds : [esp + 0x18], eax;
		popfd; // popfd时,实际上edx的值就回被修改
		popad;

		; //执行被hook覆盖的两条指令
		sub esp, ecx;
		shr ecx, 2;
		jmp g_hookAddr_next_ins;
	}
}