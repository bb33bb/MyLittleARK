#pragma once
#include "CDlgListBase.h"

/*****************************
*	Module info structure
*******************************/
typedef struct _ModInfo {
	wchar_t wszModName[0x100];
	PVOID DllBase;
	ULONG SizeOfImage;
}ModInfo, * PModInfo;



class CDlgMod :
	public CDlgListBase
{
public:

public:
	HANDLE m_hPID;
	void SetPID(HANDLE hPID);
	virtual BOOL OnInitDialog();
};

