#pragma once
#include "CDlgListBase.h"

/*****************************
*	Driver info structure
*******************************/
typedef struct _DriverInfo {
	PVOID DllBase;
	ULONG SizeOfImage;
	WCHAR FullDllName[256];
}DriverInfo, * PDriverInfo;

class CDlgDriver :
	public CDlgListBase
{
public:
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMRClickListbase(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDriverHide();
};

