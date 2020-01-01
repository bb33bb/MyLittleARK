#include "BasicModule.h"

void GetUserBuf(IRP* pIrp, void** ppInBuf, void** ppOutBuf)
{
	IO_STACK_LOCATION* pStack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG ulDeviceCtrlCode = pStack->Parameters.DeviceIoControl.IoControlCode;

	*ppInBuf = pIrp->AssociatedIrp.SystemBuffer;

	if (pIrp->MdlAddress
		&& (METHOD_FROM_CTL_CODE(ulDeviceCtrlCode) & METHOD_OUT_DIRECT))
	{
		*ppOutBuf = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, 0);
	}
	else if (pIrp->AssociatedIrp.SystemBuffer)
	{
		*ppOutBuf = pIrp->AssociatedIrp.SystemBuffer;
	}
	else
	{
		*ppOutBuf = NULL;
		KdPrint(("[**WARNING**]pBuf == NULL\n"));
	}

}


/****************************
*	Driver operations
******************************/

NTSTATUS OnGetDriversInfoSize(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDRIVER_OBJECT pDriver = pDeviceObject->DriverObject;
	P_MY_LDR_DATA_TABLE_ENTRY pLdr = pDriver->DriverSection;
	P_MY_LDR_DATA_TABLE_ENTRY pBegin = pLdr;
	unsigned int nDriver = 0;
	void* pOutBuf = NULL, * pInBuf = NULL;
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

#ifdef _DEBUG
	//KdBreakPoint();
#endif
	__try {
		do {
			++nDriver;
			pLdr = (P_MY_LDR_DATA_TABLE_ENTRY)pLdr->InLoadOrderLinks.Flink;
		} while (pBegin != pLdr);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("Exception: 0x%08X\n", GetExceptionCode()));
	}
	RtlCopyMemory(pOutBuf, &nDriver, sizeof(nDriver));

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = sizeof(nDriver);
	return status;
}

NTSTATUS OnEnumDrivers(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	PDRIVER_OBJECT pDriver = pDeviceObject->DriverObject;
	NTSTATUS status = STATUS_SUCCESS;
	P_MY_LDR_DATA_TABLE_ENTRY pLdr = pDriver->DriverSection;
	P_MY_LDR_DATA_TABLE_ENTRY pBegin = pLdr;
	PDriverInfo pDriverInfo = NULL;

	void* pOutBuf = NULL, * pInBuf = NULL;
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);
	pDriverInfo = (PDriverInfo)pOutBuf;
#ifdef _DEBUG
	//KdBreakPoint();
#endif

	__try {
		do {
			pDriverInfo->DllBase = pLdr->DllBase;
			pDriverInfo->SizeOfImage = pLdr->SizeOfImage;
			wcscpy_s(pDriverInfo->FullDllName,
				sizeof(pDriverInfo->FullDllName) / 2,
				pLdr->FullDllName.Buffer);

			++pDriverInfo;
			pLdr = (P_MY_LDR_DATA_TABLE_ENTRY)pLdr->InLoadOrderLinks.Flink;
		} while (pBegin != pLdr);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("Exception: 0x%08X\n", GetExceptionCode()));
	}

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = (unsigned int)pDriverInfo - (unsigned int)pOutBuf;
	return status;
}

NTSTATUS OnHideDriver(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	pDevice;
	void* pOutBuf = NULL, * pInBuf = NULL;
	wchar_t* pwNameDriver = NULL;
#ifdef _DEBUG
	KdBreakPoint();
#endif
	P_MY_LDR_DATA_TABLE_ENTRY pLdrEntryBegin = (P_MY_LDR_DATA_TABLE_ENTRY)
		(pDevice->DriverObject->DriverSection);
	P_MY_LDR_DATA_TABLE_ENTRY pLdrEntry = pLdrEntryBegin;
	UNICODE_STRING uStrDriverName;

	GetUserBuf(pIrp, &pInBuf, &pOutBuf);
	pwNameDriver = (wchar_t*)pOutBuf;
	RtlInitUnicodeString(&uStrDriverName, pwNameDriver);
	while ((P_MY_LDR_DATA_TABLE_ENTRY)(pLdrEntry->InLoadOrderLinks.Flink) != pLdrEntryBegin)
	{
		if (pLdrEntry->FullDllName.Buffer == NULL
			|| RtlCompareUnicodeString(&pLdrEntry->FullDllName, &uStrDriverName, FALSE) != 0)
		{
			pLdrEntry = (P_MY_LDR_DATA_TABLE_ENTRY)(pLdrEntry->InLoadOrderLinks.Flink);
			continue;
		}

		pLdrEntry->InLoadOrderLinks.Blink->Flink = pLdrEntry->InLoadOrderLinks.Flink;
		pLdrEntry->InLoadOrderLinks.Flink->Blink = pLdrEntry->InLoadOrderLinks.Blink;

		pLdrEntry->InLoadOrderLinks.Blink = &pLdrEntry->InLoadOrderLinks;
		pLdrEntry->InLoadOrderLinks.Flink = &pLdrEntry->InLoadOrderLinks;


		KdPrint(("Hide Driver %ls successfully\n", pwNameDriver));
		break;
	}
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	return status;
}


/****************************
*	Process operations
******************************/

NTSTATUS OnGetProcInfoSize(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;

#ifdef _DEBUG
	//KdBreakPoint();
#endif

	PEPROCESS pEproc = NULL;
	unsigned int nProc = 0;
	void* pOutBuf = NULL, * pInBuf = NULL;
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

	for (int i = 4; i < 0x25600; i += 4)
	{
		if (!NT_SUCCESS(PsLookupProcessByProcessId(
			(HANDLE)i, &pEproc)))
		{
			continue;
		}

		++nProc;

		// 递减一次引用计数
		ObDereferenceObject(pEproc);
	}
	RtlCopyMemory(pOutBuf, &nProc, sizeof(nProc));
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = sizeof(nProc);
	return status;
}



NTSTATUS OnEnumProcs(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;

#ifdef _DEBUG
	//KdBreakPoint();
#endif

	PEPROCESS pEproc = NULL;
	PProcInfo pProcInfo = NULL;
	void* pOutBuf = NULL, * pInBuf = NULL;
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

	pProcInfo = (PProcInfo)pOutBuf;
	for (int i = 4; i < 0x25600; i += 4)
	{
		status = PsLookupProcessByProcessId((HANDLE)i, &pEproc);
		if (!NT_SUCCESS(status))
		{
			continue;
		}
		__try {
			KdPrint(("1\n"));
			pProcInfo->pEproc = pEproc;
			pProcInfo->pid = PsGetProcessId(pEproc);
			pProcInfo->ppid = (HANDLE)PsGetProcessInheritedFromUniqueProcessId(pEproc);

			strcpy_s(pProcInfo->szProcName,
				sizeof(pProcInfo->szProcName),
				PsGetProcessImageFileName(pEproc));

			++pProcInfo;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		// 递减一次引用计数
		ObDereferenceObject(pEproc);
	}

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = (ULONG_PTR)pProcInfo - (ULONG_PTR)pOutBuf;
	return status;
}

NTSTATUS OnKillProc(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	pDeviceObject;
#ifdef _DEBUG
	//KdBreakPoint();
#endif
	void* pOutBuf = NULL, * pInBuf = NULL;
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

	HANDLE hProc = NULL;
	OBJECT_ATTRIBUTES attr = { sizeof(OBJECT_ATTRIBUTES) };
	CLIENT_ID clientID = { 0 };

	clientID.UniqueProcess = *(HANDLE*)pInBuf;
	clientID.UniqueThread = 0;
	__try {
		status = ZwOpenProcess(&hProc, 1, &attr, &clientID);
		if (hProc)
		{
			status = ZwTerminateProcess(hProc, 0);
			ZwClose(hProc);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}



	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;

	return status;
}

PEPROCESS GetNextEproc(PEPROCESS pEproc)
{
	LIST_ENTRY listEntry = ((P_MY_EPROCESS)pEproc)->ActiveProcessLinks;
	unsigned int uOffset = (ULONG)&(((P_MY_EPROCESS)0)->ActiveProcessLinks);
	PEPROCESS pNext = (PEPROCESS)((unsigned int)listEntry.Flink - uOffset);
	return pNext;
}
NTSTATUS OnHideProc(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	pDevice;
	NTSTATUS status = STATUS_SUCCESS;
	void* pOutBuf = NULL, * pInBuf = NULL;

#ifdef _DEBUG
	KdBreakPoint();
#endif
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);
	HANDLE hPID = *(HANDLE*)pInBuf;
	PEPROCESS pEprocBegin = PsGetCurrentProcess();
	PEPROCESS pEproc = pEprocBegin;
	while (((P_MY_EPROCESS)pEproc)->ActiveProcessLinks.Flink != &((P_MY_EPROCESS)pEprocBegin)->ActiveProcessLinks)
	{
		if (hPID != PsGetProcessId(pEproc))
		{
			pEproc = GetNextEproc(pEproc);
			continue;
		}

		P_MY_EPROCESS pTarget = (P_MY_EPROCESS)pEproc;
		pTarget->ActiveProcessLinks.Blink->Flink = pTarget->ActiveProcessLinks.Flink;
		pTarget->ActiveProcessLinks.Flink->Blink = pTarget->ActiveProcessLinks.Blink;

		pTarget->ActiveProcessLinks.Blink = &pTarget->ActiveProcessLinks;
		pTarget->ActiveProcessLinks.Flink = &pTarget->ActiveProcessLinks;
		break;
	}
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	return status;
}

/****************************
*	Thread operations
******************************/
PETHREAD GetEthreadFromEProc(PEPROCESS pEproc)
{
	LIST_ENTRY listEntry = ((P_MY_EPROCESS)pEproc)->ThreadListHead;
	unsigned int offset = (unsigned int)&(((P_MY_ETHREAD)0)->ThreadListEntry);
	PETHREAD pEthread = (PETHREAD)((unsigned int)listEntry.Flink - offset);
	return pEthread;
}
PETHREAD GetNextEthread(PETHREAD pEthread)
{
	LIST_ENTRY listEntry = ((P_MY_ETHREAD)pEthread)->ThreadListEntry;
	unsigned int offset = (unsigned int)&(((P_MY_ETHREAD)0)->ThreadListEntry);
	PETHREAD pNext = (PETHREAD)((unsigned int)listEntry.Flink - offset);
	return pNext;
}

NTSTATUS OnGetThreadInfoNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;

#ifdef _DEBUG
	//KdBreakPoint();
#endif

	PEPROCESS pEproc = NULL;
	PETHREAD pEthread = NULL, pEthreadHead = NULL;
	unsigned int nThread = 0;
	void* pOutBuf = NULL, * pInBuf = NULL;
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

	// Reference not increased.
	HANDLE hPID = *(HANDLE*)pInBuf;
	status = PsLookupProcessByProcessId(hPID, &pEproc);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("GetThreadInfoSize()!PsLookupProcessByProcessId() error\n\n"));
		pIrp->IoStatus.Status = status;
		pIrp->IoStatus.Information = 0;
	}
	pEthreadHead = GetEthreadFromEProc(pEproc);
	pEthread = pEthreadHead;
	do
	{
		++nThread;
		pEthread = GetNextEthread(pEthread);
	} while (GetNextEthread(pEthread) != pEthreadHead);

	ObDereferenceObject(pEproc);

	RtlCopyMemory(pOutBuf, &nThread, sizeof(nThread));
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = sizeof(nThread);
	return status;
}

NTSTATUS OnEnumThread(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;

#ifdef _DEBUG
	//KdBreakPoint();
#endif

	PEPROCESS pEproc = NULL;
	PETHREAD pEthread = NULL, pEthreadHead = NULL;
	PThreadInfo pThreadInfo = NULL;
	void* pOutBuf = NULL, * pInBuf = NULL;

	GetUserBuf(pIrp, &pInBuf, &pOutBuf);
	pThreadInfo = (PThreadInfo)pOutBuf;

	HANDLE hPID = *(HANDLE*)pInBuf;
	status = PsLookupProcessByProcessId(hPID, &pEproc);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("EnumThread()!PsLookupProcessByProcessId() error\n\n"));
		pIrp->IoStatus.Status = status;
		pIrp->IoStatus.Information = 0;
	}

	pEthreadHead = GetEthreadFromEProc(pEproc);
	pEthread = pEthreadHead;
	do
	{
		pThreadInfo->pEthread = pEthread;
		pThreadInfo->tid = PsGetThreadId(pEthread);
		++pThreadInfo;
		pEthread = GetNextEthread(pEthread);
	} while (GetNextEthread(pEthread) != pEthreadHead);

	ObDereferenceObject(pEproc);

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = (ULONG_PTR)pThreadInfo - (ULONG_PTR)pOutBuf;
	return status;
}


/****************************
*	Module operations
******************************/
NTSTATUS OnGetModInfoNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;

#ifdef _DEBUG
	//KdBreakPoint();
#endif

	HANDLE hPID = NULL;
	unsigned int nMod = 0;
	void* pOutBuf = NULL, * pInBuf = NULL;
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);
	hPID = *(HANDLE*)pInBuf;
	PEPROCESS pEproc = NULL;

	if (!NT_SUCCESS(PsLookupProcessByProcessId(hPID, &pEproc)))
	{
		KdPrint(("GetModInfoSize()!PsLookupProcessByProcessId() error\n\n"));
		pIrp->IoStatus.Status = status;
		pIrp->IoStatus.Information = 0;
	}

	KAPC_STATE kapc_state = { 0 };
	KeStackAttachProcess(pEproc, &kapc_state);

	PPEB pPEB = PsGetProcessPeb(pEproc);
	__try {
		P_MY_LDR_DATA_TABLE_ENTRY pLdr = (P_MY_LDR_DATA_TABLE_ENTRY)(pPEB->Ldr->InLoadOrderModuleList.Flink);
		P_MY_LDR_DATA_TABLE_ENTRY pLdrBegin = pLdr;
		do {
			++nMod;
			pLdr = (P_MY_LDR_DATA_TABLE_ENTRY)(pLdr->InLoadOrderLinks.Flink);
		} while (pLdr != pLdrBegin);


	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}

	KeUnstackDetachProcess(&kapc_state);
	ObDereferenceObject(pEproc);

	RtlCopyMemory(pOutBuf, &nMod, sizeof(nMod));
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = sizeof(nMod);
	return status;
}
NTSTATUS OnEnumMod(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;

#ifdef _DEBUG
	//KdBreakPoint();
#endif

	HANDLE hPID = NULL;
	PEPROCESS pEproc = NULL;
	PModInfo pModInfo = NULL;
	void* pOutBuf = NULL, * pInBuf = NULL;

	GetUserBuf(pIrp, &pInBuf, &pOutBuf);
	hPID = *(HANDLE*)pInBuf;
	pModInfo = (PModInfo)pOutBuf;

	if (!NT_SUCCESS(PsLookupProcessByProcessId(hPID, &pEproc)))
	{
		KdPrint(("GetModInfoSize()!PsLookupProcessByProcessId() error\n\n"));
		pIrp->IoStatus.Status = status;
		pIrp->IoStatus.Information = 0;
	}

	KAPC_STATE kapc_state = { 0 };
	KeStackAttachProcess(pEproc, &kapc_state);

	PPEB pPEB = PsGetProcessPeb(pEproc);
	__try {
		P_MY_LDR_DATA_TABLE_ENTRY pLdr = (P_MY_LDR_DATA_TABLE_ENTRY)(pPEB->Ldr->InLoadOrderModuleList.Flink);
		P_MY_LDR_DATA_TABLE_ENTRY pLdrBegin = pLdr;
		do {
			RtlCopyMemory(pModInfo->wszModName, pLdr->FullDllName.Buffer, pLdr->FullDllName.Length * 2);

			pModInfo->DllBase = pLdr->DllBase;
			pModInfo->SizeOfImage = pLdr->SizeOfImage;

			++pModInfo;
			pLdr = (P_MY_LDR_DATA_TABLE_ENTRY)(pLdr->InLoadOrderLinks.Flink);
		} while (pLdr != pLdrBegin);


	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}

	KeUnstackDetachProcess(&kapc_state);
	ObDereferenceObject(pEproc);

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = (ULONG_PTR)pModInfo - (ULONG_PTR)pOutBuf;
	return status;
}




/****************************
*	FIle operations
******************************/

////////////////////////////////////////

NTSTATUS FindFirstFile(const WCHAR* pszPath,
	HANDLE* phDir,
	FILE_BOTH_DIR_INFORMATION* pFileInfo,
	int nInfoSize)
{
	NTSTATUS status = STATUS_SUCCESS;
	// 1. 打开文件夹,得到文件夹的文件句柄
	HANDLE hDir = NULL;
	OBJECT_ATTRIBUTES oa = { 0 };
	UNICODE_STRING path;
	RtlInitUnicodeString(&path, pszPath);

	InitializeObjectAttributes(
		&oa,/*要初始化的对象属性结构体*/
		&path,/*文件路径*/
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,/*属性:路径不区分大小写,打开的句柄是内核句柄*/
		NULL,
		NULL);
	IO_STATUS_BLOCK isb = { 0 };
	status = ZwCreateFile(
		&hDir,/*输出的文件句柄*/
		GENERIC_READ,
		&oa,/*对象属性,需要提前将文件夹路径初始化进去*/
		&isb,
		NULL,/*文件预分配大小*/
		FILE_ATTRIBUTE_NORMAL,/*文件属性*/
		FILE_SHARE_READ,/*共享方式*/
		FILE_OPEN_IF,/*创建描述: 存在则打开*/
		FILE_DIRECTORY_FILE,/*创建选项: 目录文件*/
		NULL,
		0);

	if (!NT_SUCCESS(isb.Status)) {
		return isb.Status;
	}

	// 2. 通过文件夹的文件句柄查询文件夹下的文件信息.
	status = ZwQueryDirectoryFile(
		hDir,
		NULL,/*用于异步IO*/
		NULL,
		NULL,
		&isb,
		pFileInfo,/*保存文件信息的缓冲区*/
		nInfoSize,/*缓冲区的字节数.*/
		FileBothDirectoryInformation,/*要获取的信息的类型*/
		TRUE,/*是否只返回一个文件信息*/
		NULL,/*用于过滤文件的表达式: *.txt*/
		TRUE/*是否重新开始扫描*/
	);
	if (!NT_SUCCESS(isb.Status)) {
		return isb.Status;
	}
	// 传出文件句柄
	*phDir = hDir;
	return STATUS_SUCCESS;
}

NTSTATUS FindNextFile(HANDLE hDir,
	FILE_BOTH_DIR_INFORMATION* pFileInfo,
	int nInfoSize)
{
	IO_STATUS_BLOCK isb = { 0 };
	ZwQueryDirectoryFile(
		hDir,
		NULL,/*用于异步IO*/
		NULL,
		NULL,
		&isb,
		pFileInfo,/*保存文件信息的缓冲区*/
		nInfoSize,/*缓冲区的字节数.*/
		FileBothDirectoryInformation,/*要获取的信息的类型*/
		TRUE,/*是否只返回一个文件信息*/
		NULL,/*用于过滤文件的表达式: *.txt*/
		FALSE/*是否重新开始扫描*/
	);
	return isb.Status;
}

NTSTATUS OnGetFileNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

	HANDLE hDir = NULL;
	void* pInBuf = NULL, * pOutBuf = NULL;
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);
	wchar_t *pPath = (wchar_t*)pInBuf;
	unsigned int nFile = 0;

	char buff[sizeof(FILE_BOTH_DIR_INFORMATION) + 266 * 2];
	FILE_BOTH_DIR_INFORMATION* pFileInfo = (FILE_BOTH_DIR_INFORMATION*)buff;

	

#ifdef _DEBUG
	//KdBreakPoint();
#endif 
	status = FindFirstFile(
		pPath,
		&hDir,
		pFileInfo,
		sizeof(buff));
	if (!NT_SUCCESS(status)) {
		KdPrint(("查找第一个文件失败:%08X\n", status));
		return status;
	}

	do
	{
		KdPrint(("文件名: %ls\n", pFileInfo->FileName));
		++nFile;
	} while (STATUS_SUCCESS == FindNextFile(hDir, pFileInfo, sizeof(buff)));

	*(unsigned int*)pOutBuf = nFile;

	pIrp->IoStatus.Information = sizeof(nFile);
	pIrp->IoStatus.Status = status;
	return status;
}
NTSTATUS OnEnumFile(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

	HANDLE hDir = NULL;
	void* pInBuf = NULL, * pOutBuf = NULL;

	GetUserBuf(pIrp, &pInBuf, &pOutBuf);
	wchar_t* pPath = (wchar_t*)pInBuf;
	unsigned int nFile = 0;

	char buff[sizeof(FILE_BOTH_DIR_INFORMATION) + 266 * 2] = { 0 };
	FILE_BOTH_DIR_INFORMATION* pFileInfo = (FILE_BOTH_DIR_INFORMATION*)buff;
	FILE_BOTH_DIR_INFORMATION* pOutFileInfo = (FILE_BOTH_DIR_INFORMATION*)pOutBuf;

	
#ifdef _DEBUG
	KdBreakPoint();
#endif 
	status = FindFirstFile(
		pPath,
		&hDir,
		pFileInfo,
		sizeof(buff));
	if (!NT_SUCCESS(status)) {
		KdPrint(("查找第一个文件失败:%08X\n", status));
		return status;
	}

	do
	{
		KdPrint(("文件名: %ls\n", pFileInfo->FileName));

		RtlCopyMemory(pOutFileInfo, pFileInfo, sizeof(buff));

		RtlFillMemory(pFileInfo, 0, sizeof(buff));

		++nFile;
		pOutFileInfo = (FILE_BOTH_DIR_INFORMATION*)((unsigned int)pOutFileInfo + sizeof(buff));

	} while (STATUS_SUCCESS == FindNextFile(hDir, pFileInfo, sizeof(buff)));
	   	 	

	pIrp->IoStatus.Information = sizeof(buff) * nFile;
	pIrp->IoStatus.Status = status;
	return status;
}

/****************************
*	Registry operations
******************************/




/****************************
*	IDT operations
******************************/
NTSTATUS OnGetIdt(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;

	IDT_INFO stcIDT = { 0 };
	PIDTENTRY pIdtEntryOut = NULL, pIdtEntry = NULL;

	void* pInBuf = NULL, * pOutBuf = NULL;
	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

	__asm sidt stcIDT;

	pIdtEntryOut = (PIDTENTRY)pOutBuf;

	pIdtEntry = (PIDTENTRY)MAKE_LONG(stcIDT.uLowIdtBase, stcIDT.uHighIdtBase);
	*(unsigned int*)pIdtEntryOut = (unsigned int)pIdtEntry;

	++pIdtEntryOut;
	++pIdtEntry;
	for (unsigned int i = 0; i < 256; ++i, ++pIdtEntryOut, ++pIdtEntry)
	{
		*pIdtEntryOut = *pIdtEntry;
	}

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = (ULONG_PTR)pIdtEntryOut - (ULONG_PTR)pOutBuf;
	return status;
}





/****************************
*	GDT operations
******************************/

NTSTATUS OnGetGdtInfoNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;
	GDT_INFO stcGDT = { 0 };
	unsigned int nGdtEntry = 0;
	void* pInBuf = NULL, * pOutBuf = NULL;

	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

	__asm sgdt stcGDT;

	nGdtEntry = stcGDT.uGdtLimit / 8;
	*(unsigned int*)pOutBuf = nGdtEntry;

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = sizeof(unsigned int);

	return status;
}
NTSTATUS OnEnumGdt(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;
	GDT_INFO stcGDT = { 0 };
	PGDTENTRY pGdtEntryOut = NULL, pGdtEntry = NULL;
	unsigned int nGdtEntry = 0;
	void* pInBuf = NULL, * pOutBuf = NULL;

#ifdef _DEBUG
	//KdBreakPoint();
#endif

	GetUserBuf(pIrp, &pInBuf, &pOutBuf);
	pGdtEntryOut = (PGDTENTRY)pOutBuf;
	__asm sgdt stcGDT

	pGdtEntry = (PGDTENTRY)MAKE_LONG(stcGDT.uLowGdtBase, stcGDT.uHighGdtBase);

	// The 1st is GDT addr
	*(PGDTENTRY*)pGdtEntryOut = pGdtEntry;
	++pGdtEntryOut;

	nGdtEntry = stcGDT.uGdtLimit / 8;
	for (unsigned int i = 0; i < nGdtEntry; ++i, ++pGdtEntryOut, ++pGdtEntry)
	{
		*pGdtEntryOut = *pGdtEntry;
	}

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = (ULONG_PTR)pGdtEntryOut - (ULONG_PTR)pOutBuf;


	return status;
}




/****************************
*	SSDT operations
******************************/
NTSTATUS OnGetSSDTServiceNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;
	void* pInBuf = NULL, * pOutBuf = NULL;

	PETHREAD pEthread = PsGetCurrentThread();
	PSSDTDescriptor pSSDTDescriptor = (PSSDTDescriptor)(*(ULONG*)((ULONG)pEthread + 0xBC));

	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

	*(ULONG*)pOutBuf = pSSDTDescriptor->ntoskrnl.NumberOfService;

	pIrp->IoStatus.Information = sizeof(ULONG);
	pIrp->IoStatus.Status = status;

	return status;
}
NTSTATUS OnEnumSSDT(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;
	NTSTATUS status = STATUS_SUCCESS;
	void* pInBuf = NULL, * pOutBuf = NULL;

	PETHREAD pEthread = PsGetCurrentThread();
	PSSDTDescriptor pSSDTDescriptor = (PSSDTDescriptor)(*(ULONG*)((ULONG)pEthread + 0xBC));

#ifdef _DEBUG
	KdBreakPoint();
#endif

	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

	PULONG pTmp = (PULONG)pOutBuf;

	for (ULONG i = 0; i < pSSDTDescriptor->ntoskrnl.NumberOfService; ++i)
	{
		pTmp[i] = pSSDTDescriptor->ntoskrnl.ServiceTableBase[i];
	}

	pIrp->IoStatus.Information = sizeof(ULONG) * pSSDTDescriptor->ntoskrnl.NumberOfService;
	pIrp->IoStatus.Status = status;

	return status;
}


/*************************
*   HOOK ZwOpenProcess
* to protect a process
*************************/
PSSDTDescriptor g_pSSDTDescriptor = NULL;
PZwOpenProcess g_pOldZwOpenProcess = NULL;
ULONG g_uPID = 0;

void InstallHookSSDT()
{
	// 2. 找到函数在表中的位置(其位置就是调用号.)
	// 2.1 保存旧的函数地址(0xBE是ZwOpenProcess函数)
	g_pOldZwOpenProcess =
		(PZwOpenProcess)g_pSSDTDescriptor->ntoskrnl.ServiceTableBase[0xBE];
	// 3. 将内存分页设置为可写
	DisablePageWriteProtect();
	// 4. 写入新函数地址到SSDT表中
	g_pSSDTDescriptor->ntoskrnl.ServiceTableBase[0xBE] = (ULONG)MyZwOpenProcess;
	// 5. 将内存分页属性恢复不可写
	EnablePageWriteProtect();
}
void UninstallHookSSDT()
{
	if (!g_pOldZwOpenProcess) return;
	EnablePageWriteProtect();
	g_pSSDTDescriptor->ntoskrnl.ServiceTableBase[0xBE] = (ULONG)g_pOldZwOpenProcess;
	DisablePageWriteProtect();
}

void _declspec(naked) DisablePageWriteProtect()
{
	__asm {
		push eax;

		mov eax, cr0;
		and eax, ~0x10000;
		mov cr0, eax;

		pop eax;
		ret;
	}
}
void _declspec(naked) EnablePageWriteProtect()
{
	__asm {
		push eax;

		mov eax, cr0;
		or eax, 0x10000;
		mov cr0, eax;

		pop eax;
		ret;
	}
}

NTSTATUS NTAPI MyZwOpenProcess(
	_Out_ PHANDLE ProcessHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PCLIENT_ID ClientId
)
{

	if (ClientId->UniqueProcess == (HANDLE)g_uPID)
	{
#ifdef _DEBUG
		//KdBreakPoint();
#endif
		DesiredAccess = 0; // 将访问权限置零
		KdPrint(("Ooops! U can't Open Me~~\n"));
	}

	return g_pOldZwOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);

}

NTSTATUS OnHookOpenProcess(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	pDevice;
	NTSTATUS status = STATUS_SUCCESS;

	void* pInBuf = NULL, * pOutBuf = NULL;

#ifdef _DEBUG
	KdBreakPoint();
#endif

	GetUserBuf(pIrp, &pInBuf, &pOutBuf);

	g_uPID = *(ULONG*)pInBuf;

	// 1. 找到SSDT表的首地址
  // 1.1 在分析KiFastCallEntry时, 可以看到系统从KPCR中取出了当前线程对象
  //     然后又从当前线程对象(KTHREAD)中取出了ServiceTable.因此,可以仿照
  //     这个做法.
  // 1.2 ServiceTable在KTHREAD的以下偏移:
  //       +0x0bc ServiceTable     : Ptr32 Void
	PETHREAD pCurThread = PsGetCurrentThread();
	g_pSSDTDescriptor = (SSDTDescriptor*)
		(*(ULONG*)((ULONG_PTR)pCurThread + 0xBC));

	InstallHookSSDT();

	return status;
}