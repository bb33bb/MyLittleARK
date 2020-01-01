#include "KernelReload.h"


////////////////////////////////////////////
/////////////  �ں�������غ���   /////////////
////////////////////////////////////////////

// ����SSDTȫ�ֱ���
NTSYSAPI SSDTEntry	KeServiceDescriptorTable;


static char*		g_pNewNtKernel;		// ���ں�
static ULONG		g_ntKernelSize;		// �ں˵�ӳ���С
static SSDTEntry*	g_pNewSSDTEntry;	// ��ssdt����ڵ�ַ	
static ULONG		g_hookAddr;			// ��hookλ�õ��׵�ַ
static ULONG		g_hookAddr_next_ins;// ��hook��ָ�����һ��ָ����׵�ַ.


// ��ȡNT�ں�ģ��
// ����ȡ���Ļ����������ݱ��浽pBuff��.
NTSTATUS loadNtKernelModule( UNICODE_STRING* ntkernelPath, char** pBuff );

// �޸��ض�λ.
void fixRelocation( char* pDosHdr, ULONG curNtKernelBase );

// ���SSDT��
// char* pDos - �¼��ص��ں˶ѿռ��׵�ַ
// char* pCurKernelBase - ��ǰ����ʹ�õ��ں˵ļ��ػ�ַ
void initSSDT( char* pDos, char* pCurKernelBase );

// ��װHOOK
void installHook( );

// ж��HOOK
void uninstallHook( );

// inline hook KiFastCallEntry�ĺ���
 void myKiFastEntryHook( );


////////////////////////////////////////////
///////////////  ������ں���   //////////////
////////////////////////////////////////////
NTSTATUS OnKernelReload(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	pIrp;
	NTSTATUS status = STATUS_SUCCESS;
	DbgBreakPoint( );
	
	// 1. �ҵ��ں��ļ�·��
	// 1.1 ͨ�������ں�����ķ�ʽ���ҵ��ں���ģ��
	MY_LDR_DATA_TABLE_ENTRY* pLdr = ((MY_LDR_DATA_TABLE_ENTRY*)pDevice->DriverObject->DriverSection);
	// 1.2 �ں���ģ���������еĵ�2��.
	for(int i =0;i<2;++i)
		pLdr = (MY_LDR_DATA_TABLE_ENTRY*)pLdr->InLoadOrderLinks.Flink;
	
	g_ntKernelSize = pLdr->SizeOfImage;

	// 1.3 ���浱ǰ���ػ�ַ
	char* pCurKernelBase = (char*)pLdr->DllBase;
	
	KdPrint( ("base=%p name=%wZ\n", pCurKernelBase, &pLdr->FullDllName) );
	
	// 2. ��ȡntģ����ļ����ݵ��ѿռ�.
	status = loadNtKernelModule( &pLdr->FullDllName, &g_pNewNtKernel );
	if(STATUS_SUCCESS != status)
	{
		return status;
	}

	// 3. �޸���ntģ����ض�λ.
	fixRelocation( g_pNewNtKernel, (ULONG)pCurKernelBase );

	// 4. ʹ�õ�ǰ����ʹ�õ��ں˵����������
	//    ���ں˵�SSDT��.
	initSSDT( g_pNewNtKernel, pCurKernelBase );

	// 5. HOOK KiFastCallEntry,ʹ���ú������ں˵�·��
	installHook( );

	pDevice->DriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}


VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	UNREFERENCED_PARAMETER(pDriver);
	uninstallHook( );
}



// �ر��ڴ�ҳд�뱣��
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

// �����ڴ�ҳд�뱣��
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


// ����NT�ں�ģ��
// ����ȡ���Ļ����������ݱ��浽pBuff��.
NTSTATUS loadNtKernelModule( UNICODE_STRING* ntkernelPath, char** pOut )
{
	NTSTATUS status = STATUS_SUCCESS;
	// 2. ��ȡ�ļ��е��ں�ģ��
	// 2.1 ���ں�ģ����Ϊ�ļ�����.
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
		KdPrint( ("���ļ�ʧ��\n") );
		goto _DONE;
	}

	// 2.2 ��PE�ļ�ͷ����ȡ���ڴ�
	status = readFile( hFile, 0, 0, 0x1000, pKernelBuff, &read );
	if(STATUS_SUCCESS != status)
	{
		KdPrint( ("��ȡ�ļ�����ʧ��\n") );
		goto _DONE;
	}

	// 3. ����PE�ļ����ڴ�.
	// 3.1 �õ���չͷ,��ȡӳ���С. 
	IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pKernelBuff;
	IMAGE_NT_HEADERS* pnt = (IMAGE_NT_HEADERS*)((ULONG)pDos + pDos->e_lfanew);
	ULONG imgSize = pnt->OptionalHeader.SizeOfImage;
	
	// 3.2 �����ڴ��Ա���������ε�����.
	pBuff = ExAllocatePool( NonPagedPool, imgSize );
	if(pBuff == NULL)
	{
		KdPrint( ("�ڴ�����ʧ��ʧ��\n") );
		status = STATUS_BUFFER_ALL_ZEROS;	// ��㷵�ظ�������
		goto _DONE;
	}

	// 3.2.1 ����ͷ�����ѿռ�
	RtlCopyMemory( pBuff, 
				   pKernelBuff, 
				   pnt->OptionalHeader.SizeOfHeaders );

	// 3.3 �õ�����ͷ, ������������ͷ�����ζ�ȡ���ڴ���.
	IMAGE_SECTION_HEADER* pScnHdr = IMAGE_FIRST_SECTION( pnt );
	ULONG scnCount = pnt->FileHeader.NumberOfSections;
	for(ULONG i = 0; i < scnCount; ++i)
	{
		//
		// 3.3.1 ��ȡ�ļ����ݵ��ѿռ�ָ��λ��.
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
	// �������ں˵ļ��ص��׵�ַ
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


// �޸��ض�λ.
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
			// ��ȥĬ�ϼ��ػ�ַ
			//
			*pFixAddr -= pNt->OptionalHeader.ImageBase;
			
			//
			// �����µļ��ػ�ַ(ʹ�õ��ǵ�ǰ�ں˵ļ��ػ�ַ,������
			// ��Ϊ�������ں�ʹ�õ�ǰ�ں˵�����(ȫ�ֱ���,δ��ʼ������������).)
			//
			*pFixAddr += (ULONG)curNtKernelBase;
		}

		pRel = (IMAGE_BASE_RELOCATION*)((ULONG)pRel + pRel->SizeOfBlock);
	}
}

// ���SSDT��
// char* pNewBase - �¼��ص��ں˶ѿռ��׵�ַ
// char* pCurKernelBase - ��ǰ����ʹ�õ��ں˵ļ��ػ�ַ
void initSSDT( char* pNewBase, char* pCurKernelBase )
{
	// 1. �ֱ��ȡ��ǰ�ں�,�¼��ص��ں˵�`KeServiceDescriptorTable`
	//    �ĵ�ַ
	SSDTEntry* pCurSSDTEnt = &KeServiceDescriptorTable;
	g_pNewSSDTEntry = (SSDTEntry*)
		((ULONG)pCurSSDTEnt - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2. ��ȡ�¼��ص��ں��������ű�ĵ�ַ:
	// 2.1 ���������ַ
	g_pNewSSDTEntry->ServiceTableBase = (ULONG*)
		((ULONG)pCurSSDTEnt->ServiceTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.3 �����������ֽ������ַ
	g_pNewSSDTEntry->ParamTableBase = (UCHAR*)
		((ULONG)pCurSSDTEnt->ParamTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.2 ���������ô������ַ(��ʱ�������������.)
	if(pCurSSDTEnt->ServiceCounterTableBase)
	{
		g_pNewSSDTEntry->ServiceCounterTableBase = (ULONG*)
			((ULONG)pCurSSDTEnt->ServiceCounterTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);
	}

	// 2.3 ������SSDT��ķ������
	g_pNewSSDTEntry->NumberOfServices = pCurSSDTEnt->NumberOfServices;

	//3. ���������ĵ�ַ��䵽��SSDT��(�ض�λʱ��ʵ�Ѿ��޸�����,
	//   ����,���޸��ض�λ��ʱ��,��ʹ�õ�ǰ�ں˵ļ��ػ�ַ��, �޸��ض�λ
	//   Ϊ֮��, ���ں˵�SSDT����ķ������ĵ�ַ���ǵ�ǰ�ں˵ĵ�ַ,
	//   ������Ҫ����Щ�������ĵ�ַ�Ļ����ں��еĺ�����ַ.)
	disablePageWriteProtect( );
	for(ULONG i = 0; i < g_pNewSSDTEntry->NumberOfServices; ++i)
	{
		// ��ȥ��ǰ�ں˵ļ��ػ�ַ
		g_pNewSSDTEntry->ServiceTableBase[i] -= (ULONG)pCurKernelBase;
		// �������ں˵ļ��ػ�ַ.
		g_pNewSSDTEntry->ServiceTableBase[i] += (ULONG)pNewBase;
	}
	enablePageWriteProtect( );
}

void installHook( )
{
	g_hookAddr = 0;

	// 1. �ҵ�KiFastCallEntry�����׵�ַ
	ULONG uKiFastCallEntry = 0;
	_asm
	{
		;// KiFastCallEntry������ַ����
		;// ������ģ��Ĵ�����0x176�żĴ�����
		push ecx;
		push eax;
		push edx;
		mov ecx, 0x176; // ���ñ��
		rdmsr; ;// ��ȡ��edx:eax
		mov uKiFastCallEntry, eax;// ���浽����
		pop edx;
		pop eax;
		pop ecx;
	}

	// 2. �ҵ�HOOK��λ��, ������5�ֽڵ�����
	// 2.1 HOOK��λ��ѡ��Ϊ(�˴�����5�ֽ�,):
	//  2be1            sub     esp, ecx ;
	// 	c1e902          shr     ecx, 2   ;
	UCHAR hookCode[5] = { 0x2b,0xe1,0xc1,0xe9,0x02 }; //����inline hook���ǵ�5�ֽ�
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
		KdPrint( ("��KiFastCallEntry������û���ҵ�HOOKλ��,����KiFastCallEntry�Ѿ���HOOK����\n") );
		uninstallHook( );
		return;
	}


	g_hookAddr = uKiFastCallEntry + i;
	g_hookAddr_next_ins = g_hookAddr + 5;

	// 3. ��ʼinline hook
	UCHAR jmpCode[5] = { 0xe9 };// jmp xxxx
	disablePageWriteProtect( );

	// 3.1 ������תƫ��
	// ��תƫ�� = Ŀ���ַ - ��ǰ��ַ - 5
	*(ULONG*)(jmpCode + 1) = (ULONG)myKiFastEntryHook - g_hookAddr - 5;

	// 3.2 ����תָ��д��
	RtlCopyMemory( (void*)(uKiFastCallEntry + i),
				   jmpCode,
				   5 );
	
	enablePageWriteProtect( );
}

void uninstallHook( )
{
	if(g_hookAddr)
	{
		
		// ��ԭʼ����д��.
		UCHAR srcCode[5] = { 0x2b,0xe1,0xc1,0xe9,0x02 }; 
		disablePageWriteProtect( );

		// 3.1 ������תƫ��
		// ��תƫ�� = Ŀ���ַ - ��ǰ��ַ - 5

		_asm sti
		// 3.2 ����תָ��д��
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


// SSDT���˺���.
ULONG SSDTFilter( ULONG index,/*������,Ҳ�ǵ��ú�*/
				  ULONG tableAddress,/*��ĵ�ַ,������SSDT��ĵ�ַ,Ҳ������Shadow SSDT��ĵ�ַ*/
				  ULONG funAddr/*�ӱ���ȡ���ĺ�����ַ*/ )
{
	// �����SSDT��Ļ�
	if(tableAddress == (ULONG)KeServiceDescriptorTable.ServiceTableBase)
	{
		// �жϵ��ú�(190��ZwOpenProcess�����ĵ��ú�)
		if(index == 190)
		{
			// ������SSDT��ĺ�����ַ
			// Ҳ�������ں˵ĺ�����ַ.
			return g_pNewSSDTEntry->ServiceTableBase[190];
		}
	}
	// ���ؾɵĺ�����ַ
	return funAddr;
}

// inline hook KiFastCallEntry�ĺ���
void _declspec(naked) myKiFastEntryHook( )
{

	_asm
	{
		pushad; // ѹջ�Ĵ���: eax,ecx,edx,ebx, esp,ebp ,esi, edi
		pushfd; // ѹջ��־�Ĵ���

		push edx; // �ӱ���ȡ���ĺ�����ַ
		push edi; // ��ĵ�ַ
		push eax; // ���ú�
		call SSDTFilter; // ���ù��˺���

		;// �����������֮��ջ�ؼ�����,ָ��pushad��
		;// 32λ��ͨ�üĴ���������ջ��,ջ�ռ䲼��Ϊ:
		;// [esp + 00] <=> eflag
		;// [esp + 04] <=> edi
		;// [esp + 08] <=> esi
		;// [esp + 0C] <=> ebp
		;// [esp + 10] <=> esp
		;// [esp + 14] <=> ebx
		;// [esp + 18] <=> edx <<-- ʹ�ú�������ֵ���޸����λ��
		;// [esp + 1C] <=> ecx
		;// [esp + 20] <=> eax
		mov dword ptr ds : [esp + 0x18], eax;
		popfd; // popfdʱ,ʵ����edx��ֵ�ͻر��޸�
		popad;

		; //ִ�б�hook���ǵ�����ָ��
		sub esp, ecx;
		shr ecx, 2;
		jmp g_hookAddr_next_ins;
	}
}