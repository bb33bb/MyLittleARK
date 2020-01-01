#include "pch.h"
#include "CDlgDriver.h"
#include "resource.h"

BOOL CDlgDriver::OnInitDialog()
{
	CDlgListBase::OnInitDialog();

	// TODO:  在此添加额外的初始化
	DWORD dwOldStyle = m_listCtrl.GetExtendedStyle();

	SetWindowText(L"Driver");

	m_listCtrl.SetExtendedStyle(dwOldStyle
		| LVS_EX_GRIDLINES
		| LVS_EX_FULLROWSELECT);

	m_listCtrl.InsertColumn(0, L"Base", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(1, L"Size/kB", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(2, L"Name", LVCFMT_CENTER, 200);

	unsigned int nDriver = 0;
	DWORD dwSize = 0;
	DeviceIoControl(m_hDev,
		DEVICE_CTRL_CODE_GET_DRIVER_NUM,
		NULL,
		0,
		&nDriver,
		sizeof(nDriver),
		&dwSize,
		NULL);

	PDriverInfo pDriverInfo = (PDriverInfo)calloc(nDriver, sizeof(DriverInfo));

	DeviceIoControl(m_hDev,
		DEVICE_CTRL_CODE_ENUM_DRIVERS,
		NULL,
		0,
		pDriverInfo,
		nDriver * sizeof(DriverInfo),
		&dwSize,
		NULL);

	wchar_t wszBuf[20] = { 0 };
	PDriverInfo pTmp = pDriverInfo;
	for (int i = 0; i < nDriver; ++i, ++pTmp)
	{
		m_listCtrl.InsertItem(i, L"");

		swprintf_s(wszBuf, L"0x%p", pTmp->DllBase);
		m_listCtrl.SetItemText(i, 0, wszBuf);
		memset(wszBuf, 0, sizeof(wszBuf));

		swprintf_s(wszBuf, L"0x%08X", pTmp->SizeOfImage / 1024);
		m_listCtrl.SetItemText(i, 1, wszBuf);
		memset(wszBuf, 0, sizeof(wszBuf));

		m_listCtrl.SetItemText(i, 2, pTmp->FullDllName);

	}

	if (pDriverInfo) {
		free(pDriverInfo);
		pDriverInfo = NULL;
	}


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
BEGIN_MESSAGE_MAP(CDlgDriver, CDlgListBase)
	ON_NOTIFY(NM_RCLICK, IDC_LISTBASE, &CDlgDriver::OnNMRClickListbase)
	ON_COMMAND(ID_DRIVER_HIDE, &CDlgDriver::OnDriverHide)
END_MESSAGE_MAP()


void CDlgDriver::OnNMRClickListbase(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CMenu *pMenu = new CMenu;
	pMenu->LoadMenuW(IDR_MENUBASE);
	CMenu* pSubMenu = pMenu->GetSubMenu(1);

	CPoint point = { 0 };
	GetCursorPos(&point);
	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, NULL);

	*pResult = 0;
}


void CDlgDriver::OnDriverHide()
{
	// TODO: 在此添加命令处理程序代码
	 unsigned int index = (unsigned int)m_listCtrl.GetFirstSelectedItemPosition();
	 if (index == 0) return;
	 --index;

	// Driver Name
	CStringW strW = m_listCtrl.GetItemText(index, 2);
	DWORD dwSize = 0;
	LPWSTR lpwDriverName = strW.GetBuffer();
	int nDriverNameLen = strW.GetLength();
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_HIDE_DRIVERS,
		lpwDriverName,
		(nDriverNameLen +1)* 2,
		NULL,
		0,
		&dwSize,
		NULL);

}
