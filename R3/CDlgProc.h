#pragma once
#include "CDlgListBase.h"


/*****************************
*	Process info structure
*******************************/
typedef struct _ProcInfo {
	void* pEproc;
	ULONG pid;
	ULONG ppid;
	char szProcName[256];
}ProcInfo, * PProcInfo;


class CDlgProc :
	public CDlgListBase
{
public:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMRClickListbase(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnProcThreads();
	afx_msg void OnProcModules();
	afx_msg void OnProcKill();
	afx_msg void OnProcHide();
};

