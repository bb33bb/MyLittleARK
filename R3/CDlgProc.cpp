#include "pch.h"
#include "CDlgProc.h"
#include "resource.h"


#include "CDlgThread.h"
#include "CDlgMod.h"

BOOL CDlgProc::OnInitDialog()
{
	CDlgListBase::OnInitDialog();

	// TODO:  在此添加额外的初始化
	DWORD dwOldStyle = m_listCtrl.GetExtendedStyle();

	SetWindowText(L"Process");

	m_listCtrl.SetExtendedStyle(dwOldStyle
		| LVS_EX_GRIDLINES
		| LVS_EX_FULLROWSELECT);


	m_listCtrl.InsertColumn(0, L"EPROCESS", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(1, L"PID", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(2, L"PPID", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(3, L"ProcName", LVCFMT_CENTER, 150);

	unsigned int nProc = 0;
	DWORD dwSize = NULL;
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_GET_PROC_NUM,
		NULL,
		0,
		&nProc,
		sizeof(nProc),
		&dwSize,
		NULL);
	nProc += 100;
	PProcInfo pProc = (PProcInfo)calloc(nProc, sizeof(ProcInfo));
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_ENUM_PROC,
		NULL,
		0,
		pProc,
		nProc * sizeof(ProcInfo),
		&dwSize,
		NULL);

	PProcInfo pTmp = pProc;
	wchar_t wszBuf[20] = { 0 };


	//	//////////////////////////////////////////
	CStringA strA;
	CStringW strW;
	for (unsigned int i = 0; i < nProc && (*(DWORD*)pTmp != 0); ++i, ++pTmp)
	{
		m_listCtrl.InsertItem(i, L"");

		swprintf_s(wszBuf, L"0x%p", pTmp->pEproc);
		m_listCtrl.SetItemText(i, 0, wszBuf);
		memset(wszBuf, 0, sizeof(wszBuf));

		swprintf_s(wszBuf, L"%u", pTmp->pid);
		m_listCtrl.SetItemText(i, 1, wszBuf);
		memset(wszBuf, 0, sizeof(wszBuf));

		swprintf_s(wszBuf, L"%u", pTmp->ppid);
		m_listCtrl.SetItemText(i, 2, wszBuf);
		memset(wszBuf, 0, sizeof(wszBuf));

		strA = pTmp->szProcName;
		strW = strA;
		m_listCtrl.SetItemText(i, 3, strW);
	}

	if (pProc)
	{
		free(pProc);
		pProc = NULL;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
BEGIN_MESSAGE_MAP(CDlgProc, CDlgListBase)
	ON_NOTIFY(NM_RCLICK, IDC_LISTBASE, &CDlgProc::OnNMRClickListbase)
	ON_COMMAND(ID_PROC_THREADS, &CDlgProc::OnProcThreads)
	ON_COMMAND(ID_PROC_MODULES, &CDlgProc::OnProcModules)
	ON_COMMAND(ID_PROC_KILL, &CDlgProc::OnProcKill)
	ON_COMMAND(ID_PROC_HIDE, &CDlgProc::OnProcHide)
END_MESSAGE_MAP()


void CDlgProc::OnNMRClickListbase(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CMenu* pMenu = new CMenu;
	pMenu->LoadMenuW(IDR_MENUBASE);
	CMenu* pSubMenu = pMenu->GetSubMenu(0);

	CPoint point = { 0 };
	GetCursorPos(&point);
	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, NULL);

	*pResult = 0;
}


void CDlgProc::OnProcThreads()
{
	// TODO: 在此添加命令处理程序代码
	int index = (int)m_listCtrl.GetFirstSelectedItemPosition();

	if (index == 0) return;

	--index;
	CDlgThread* pDlgThread = new CDlgThread();
	CStringW strPID = m_listCtrl.GetItemText(index, 1);
	HANDLE hPID = (HANDLE)_wtoi(strPID);
	pDlgThread->SetDev(m_hDev);
	pDlgThread->SetPID(hPID);
	pDlgThread->DoModal();
}


void CDlgProc::OnProcModules()
{
	// TODO: 在此添加命令处理程序代码
	int index = (int)m_listCtrl.GetFirstSelectedItemPosition();

	if (index == 0) return;

	--index;
	CDlgMod* pDlgMod = new CDlgMod();
	CStringW strPID = m_listCtrl.GetItemText(index, 1);
	HANDLE hPID = (HANDLE)_wtoi(strPID);
	pDlgMod->SetDev(m_hDev);
	pDlgMod->SetPID(hPID);
	pDlgMod->DoModal();
}


void CDlgProc::OnProcKill()
{
	// TODO: 在此添加命令处理程序代码
	int index = (int)m_listCtrl.GetFirstSelectedItemPosition();

	if (index == 0) return;

	--index;
	CDlgThread* pDlgThread = new CDlgThread();
	CStringW strPID = m_listCtrl.GetItemText(index, 1);
	HANDLE hPID = (HANDLE)_wtoi(strPID);
	DWORD dwSize = 0;
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_KILL_PROC,
		&hPID,
		sizeof(hPID),
		NULL,
		0,
		&dwSize,
		NULL);
}


void CDlgProc::OnProcHide()
{
	// TODO: 在此添加命令处理程序代码
	int index = (int)m_listCtrl.GetFirstSelectedItemPosition();

	if (index == 0) return;

	--index;
	CDlgMod* pDlgMod = new CDlgMod();
	CStringW strPID = m_listCtrl.GetItemText(index, 1);
	HANDLE hPID = (HANDLE)_wtoi(strPID);
	DWORD dwSize = 0;
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_HIDE_PROC,
		&hPID,
		sizeof(hPID),
		NULL,
		0,
		&dwSize,
		NULL);

}
