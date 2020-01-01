#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include "WinStruct.h"

#define _DEBUG


void GetUserBuf(IRP* pIrp, void** ppInBuf, void** ppOutBuf);

/****************************
*	Driver operations
******************************/
typedef struct _DriverInfo {
    PVOID DllBase;
    ULONG SizeOfImage;
    WCHAR FullDllName[256];
}DriverInfo, * PDriverInfo;


NTSTATUS OnGetDriversInfoSize(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
NTSTATUS OnEnumDrivers(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
NTSTATUS OnHideDriver(DEVICE_OBJECT *pDevice, IRP *pIrp);


/****************************
*	Process operations
******************************/

typedef struct _ProcInfo {
    void* pEproc;
    HANDLE pid;
    HANDLE ppid;
    char szProcName[256];
}ProcInfo, * PProcInfo;

char* PsGetProcessImageFileName(PEPROCESS pEproc);
NTKERNELAPI UINT32 PsGetProcessInheritedFromUniqueProcessId(PEPROCESS pEproc);

NTSTATUS OnGetProcInfoSize(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
NTSTATUS OnEnumProcs(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);

NTSTATUS OnKillProc(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);

NTSTATUS OnHideProc(DEVICE_OBJECT* pDevice, IRP* pIrp);

/*****************************
*	Thread operations
*******************************/

typedef struct _ThreadInfo {
    void* pEthread;
    HANDLE tid;
}ThreadInfo, * PThreadInfo;

PETHREAD GetEthreadFromEProc(PEPROCESS pEproc);
PETHREAD GetNextEthread(PETHREAD pEthread);
PETHREAD PsGetCurrentThread();

NTSTATUS OnGetThreadInfoNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
NTSTATUS OnEnumThread(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);


/****************************
*	Module operations
******************************/
typedef struct _ModInfo {
    wchar_t wszModName[0x100];
    PVOID DllBase;
    ULONG SizeOfImage;
}ModInfo, * PModInfo;


NTKERNELAPI struct _PEB* PsGetProcessPeb(PEPROCESS proc);

NTSTATUS OnGetModInfoNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
NTSTATUS OnEnumMod(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);



/****************************
*	FIle operations
******************************/
NTSTATUS OnGetFileNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
NTSTATUS OnEnumFile(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);


/****************************
*	Registry operations
******************************/




/****************************
*	IDT operations
******************************/

#define MAKE_LONG(a,b) ((LONG)(((UINT16)(((DWORD_PTR)(a)) & 0xffff)) | ((UINT32)((UINT16)(((DWORD_PTR)(b)) & 0xffff))) << 16))


NTSTATUS OnGetIdt(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);



/****************************
*	GDT operations
******************************/

NTSTATUS OnGetGdtInfoNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
NTSTATUS OnEnumGdt(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);





/****************************
*	SSDT operations
******************************/


NTSTATUS OnGetSSDTServiceNum(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
NTSTATUS OnEnumSSDT(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);




/*************************
*   HOOK ZwOpenProcess
* to protect a process
*************************/

typedef NTSTATUS (NTAPI *PZwOpenProcess)(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
);
extern PSSDTDescriptor g_pSSDTDescriptor;
extern PZwOpenProcess g_pOldZwOpenProcess;
extern ULONG g_uPID;

void InstallHookSSDT();
void UninstallHookSSDT();
// 关闭内存页写入保护
void DisablePageWriteProtect();
// 开启内存页写入保护
void EnablePageWriteProtect();

NTSTATUS NTAPI MyZwOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
);

NTSTATUS OnHookOpenProcess(DEVICE_OBJECT* pDevice, IRP* pIrp);