#pragma once
#include "CDlgListBase.h"


/*****************************
*	Thread info structure
*******************************/
typedef struct _ThreadInfo {
	void* pEthread;
	HANDLE tid;
}ThreadInfo, * PThreadInfo;



class CDlgThread :
	public CDlgListBase
{
public:
	HANDLE m_hPID;
	void SetPID(HANDLE hPID);
	virtual BOOL OnInitDialog();
};

