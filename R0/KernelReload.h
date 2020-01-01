#pragma once
#include <ntifs.h>
#include <ntimage.h>
#include <ntddk.h>
#include "kernelFunction.h"

NTSTATUS OnKernelReload(DEVICE_OBJECT* pDevice, IRP* pIrp);
