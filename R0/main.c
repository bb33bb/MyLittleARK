#include "BasicModule.h"
#include "KernelReload.h"

#define NAME_DEVICE	L"\\Device\\mydevice"
#define NAME_SYMBOL	L"\\DosDevices\\mysymbol"


/*******************************
*	DeviceIoControl code
*********************************/
#define MY_CTL_CODE(code, method) CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800+(code), method, FILE_ANY_ACCESS)
typedef enum _MyCtlCode {
	DEVICE_CTRL_CODE_GET_DRIVER_NUM = MY_CTL_CODE(0, METHOD_BUFFERED),
	DEVICE_CTRL_CODE_ENUM_DRIVERS = MY_CTL_CODE(1, METHOD_BUFFERED),
	DEVICE_CTRL_CODE_HIDE_DRIVERS = MY_CTL_CODE(2, METHOD_BUFFERED),

	DEVICE_CTRL_CODE_GET_PROC_NUM = MY_CTL_CODE(3, METHOD_BUFFERED),
	DEVICE_CTRL_CODE_ENUM_PROC = MY_CTL_CODE(4, METHOD_OUT_DIRECT),
	DEVICE_CTRL_CODE_HIDE_PROC = MY_CTL_CODE(5, METHOD_OUT_DIRECT),

	DEVICE_CTRL_CODE_GET_THREAD_NUM = MY_CTL_CODE(6, METHOD_BUFFERED),
	DEVICE_CTRL_CODE_ENUM_THREAD = MY_CTL_CODE(7, METHOD_OUT_DIRECT),
	DEVICE_CTRL_CODE_GET_MOD_NUM = MY_CTL_CODE(8, METHOD_BUFFERED),
	DEVICE_CTRL_CODE_ENUM_MOD = MY_CTL_CODE(9, METHOD_OUT_DIRECT),

	DEVICE_CTRL_CODE_KILL_PROC = MY_CTL_CODE(10, METHOD_BUFFERED),

	DEVICE_CTRL_CODE_IDT = MY_CTL_CODE(11, METHOD_OUT_DIRECT),

	DEVICE_CTRL_CODE_GET_GDT_NUM = MY_CTL_CODE(12, METHOD_BUFFERED),
	DEVICE_CTRL_CODE_ENUM_GDT = MY_CTL_CODE(13, METHOD_OUT_DIRECT),

	DEVICE_CTRL_CODE_GET_SSDT_SERVICE_NUM = MY_CTL_CODE(14, METHOD_OUT_DIRECT),
	DEVICE_CTRL_CODE_GET_SSDT_SERVICE = MY_CTL_CODE(15, METHOD_OUT_DIRECT),

	DEVICE_CTRL_CODE_PROTECT_SELF = MY_CTL_CODE(16, METHOD_OUT_DIRECT),

	DEVICE_CTRL_CODE_KERNEL_RELOAD = MY_CTL_CODE(17, METHOD_OUT_DIRECT),

	DEVICE_CTRL_CODE_GET_FILE_COUNT = MY_CTL_CODE(18, METHOD_BUFFERED),
	DEVICE_CTRL_CODE_GET_FILE_INFO = MY_CTL_CODE(19, METHOD_OUT_DIRECT)
}MyCtlCode;

typedef struct _DeviceIoCtrlHandler {
	MyCtlCode ulCtrlCode;
	NTSTATUS(*callback)(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
}DeviceIoCtrlHandler;

DeviceIoCtrlHandler g_DeviceIoCtrlHandlers[] = {
	{DEVICE_CTRL_CODE_GET_DRIVER_NUM, OnGetDriversInfoSize},
	{DEVICE_CTRL_CODE_ENUM_DRIVERS, OnEnumDrivers},
	{DEVICE_CTRL_CODE_HIDE_DRIVERS, OnHideDriver},

	{DEVICE_CTRL_CODE_GET_PROC_NUM, OnGetProcInfoSize},
	{DEVICE_CTRL_CODE_ENUM_PROC, OnEnumProcs},
	{DEVICE_CTRL_CODE_HIDE_PROC, OnHideProc},
	{DEVICE_CTRL_CODE_GET_THREAD_NUM, OnGetThreadInfoNum},
	{DEVICE_CTRL_CODE_ENUM_THREAD, OnEnumThread},
	{DEVICE_CTRL_CODE_GET_MOD_NUM, OnGetModInfoNum},
	{DEVICE_CTRL_CODE_ENUM_MOD, OnEnumMod},

	{DEVICE_CTRL_CODE_KILL_PROC, OnKillProc},

	{DEVICE_CTRL_CODE_IDT, OnGetIdt},

	{DEVICE_CTRL_CODE_GET_GDT_NUM, OnGetGdtInfoNum},
	{DEVICE_CTRL_CODE_ENUM_GDT, OnEnumGdt},

	{DEVICE_CTRL_CODE_GET_SSDT_SERVICE_NUM, OnGetSSDTServiceNum},
	{DEVICE_CTRL_CODE_GET_SSDT_SERVICE, OnEnumSSDT},

	{DEVICE_CTRL_CODE_PROTECT_SELF, OnHookOpenProcess},

	{DEVICE_CTRL_CODE_KERNEL_RELOAD, OnKernelReload},

	{DEVICE_CTRL_CODE_GET_FILE_COUNT, OnGetFileNum},
	{DEVICE_CTRL_CODE_GET_FILE_INFO, OnEnumFile}
};


/***********************************
*	Dispatch functions declaration
************************************/

void OnUnload(DRIVER_OBJECT* pDriver);

NTSTATUS OnCommonDispatch(DEVICE_OBJECT* pDeviceObject, IRP* pIrp);
NTSTATUS OnDeviceIoControl(DEVICE_OBJECT* pDevice, IRP* pIrp);

NTSTATUS DriverEntry(DRIVER_OBJECT* pDriver, UNICODE_STRING* strRegPath)
{
	strRegPath;
	NTSTATUS status = STATUS_SUCCESS;
	pDriver->DriverUnload = OnUnload;

	UNICODE_STRING uStrNameDevice = RTL_CONSTANT_STRING(NAME_DEVICE);
	PDEVICE_OBJECT pDevice = NULL;
	KdBreakPoint();


	status = IoCreateDevice(
		pDriver,
		0,
		&uStrNameDevice,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&pDevice);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("CreateDevice() error: 0x%08x\n", status));
		return status;
	}
	pDevice->Flags |= DO_DIRECT_IO;


	UNICODE_STRING uStrNameSymbol = RTL_CONSTANT_STRING(NAME_SYMBOL);
	status = IoCreateSymbolicLink(
		&uStrNameSymbol,
		&uStrNameDevice);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("IoCreateSymbolicLink() error: 0x%08x\n", status));
		return status;
	}

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriver->MajorFunction[i] = OnCommonDispatch;
	}
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceIoControl;

	return status;
}


void OnUnload(DRIVER_OBJECT* pDriver)
{
	KdPrint(("unload deriver\n"));
	
	UninstallHookSSDT();

	// 删除符号链接
	UNICODE_STRING symbolName = RTL_CONSTANT_STRING(NAME_SYMBOL);
	IoDeleteSymbolicLink(&symbolName);
	
	// 销毁设备对象.
	IoDeleteDevice(pDriver->DeviceObject);
}

NTSTATUS OnCommonDispatch(DEVICE_OBJECT* pDeviceObject, IRP* pIrp)
{
	pDeviceObject;

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0; // 完成字节数
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


NTSTATUS OnDeviceIoControl(DEVICE_OBJECT* pDevice, IRP* pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

	// 通过IRP栈获取用户层参数
	IO_STACK_LOCATION* pStack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG ulDeviceCtrlCode = pStack->Parameters.DeviceIoControl.IoControlCode;
	ULONG ulInBufLen = pStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG ulOutBufLen = pStack->Parameters.DeviceIoControl.OutputBufferLength;

	KdPrint(("控制码:%08X 输入长度:%d 输出长度:%d\n",
		ulDeviceCtrlCode,
		ulInBufLen,
		ulOutBufLen));

	unsigned int nCode = sizeof(g_DeviceIoCtrlHandlers) / sizeof(g_DeviceIoCtrlHandlers[0]);
	for (unsigned int i = 0; i < nCode; ++i) {
		if ((ULONG)(g_DeviceIoCtrlHandlers[i].ulCtrlCode) == ulDeviceCtrlCode) {
			status = g_DeviceIoCtrlHandlers[i].callback(pDevice, pIrp);
			break;
		}
	}

	

	//pIrp->IoStatus.Status = status;
	//pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


