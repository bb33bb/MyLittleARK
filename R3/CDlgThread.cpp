#include "pch.h"
#include "CDlgThread.h"

void CDlgThread::SetPID(HANDLE hPID)
{
	m_hPID = hPID;
}


BOOL CDlgThread::OnInitDialog()
{
	CDlgListBase::OnInitDialog();

	// TODO:  在此添加额外的初始化
	DWORD dwOldStyle = m_listCtrl.GetExtendedStyle();

	SetWindowText(L"Thread");

	m_listCtrl.SetExtendedStyle(dwOldStyle
		| LVS_EX_GRIDLINES
		| LVS_EX_FULLROWSELECT);


	m_listCtrl.InsertColumn(0, L"ETHREAD", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(1, L"TID", LVCFMT_CENTER, 150);


	unsigned int nThread = 0;
	DWORD dwSize = NULL;
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_GET_THREAD_NUM,
		&m_hPID,
		sizeof(m_hPID),
		&nThread,
		sizeof(nThread),
		&dwSize,
		NULL);

	PThreadInfo pThreadInfo = (PThreadInfo)calloc(nThread, sizeof(ThreadInfo));
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_ENUM_THREAD,
		&m_hPID,
		sizeof(m_hPID),
		pThreadInfo,
		nThread * sizeof(ThreadInfo),
		&dwSize,
		NULL);

	PThreadInfo pTmp = pThreadInfo;
	wchar_t wszBuf[20] = { 0 };

	//	//////////////////////////////////////////

	CStringW strW;
	for (unsigned int i = 0; i < nThread && (*(DWORD*)pTmp != 0); ++i, ++pTmp)
	{
		m_listCtrl.InsertItem(i, L"");

		swprintf_s(wszBuf, L"0x%p", pTmp->pEthread);
		m_listCtrl.SetItemText(i, 0, wszBuf);
		memset(wszBuf, 0, sizeof(wszBuf));

		swprintf_s(wszBuf, L"%u", pTmp->tid);
		m_listCtrl.SetItemText(i, 1, wszBuf);
		memset(wszBuf, 0, sizeof(wszBuf));


	}

	if (pThreadInfo)
	{
		free(pThreadInfo);
		pThreadInfo = NULL;
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
