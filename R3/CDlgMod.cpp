#include "pch.h"
#include "CDlgMod.h"




void CDlgMod::SetPID(HANDLE hPID)
{
	m_hPID = hPID;
}




BOOL CDlgMod::OnInitDialog()
{
	CDlgListBase::OnInitDialog();

	// TODO:  在此添加额外的初始化
	DWORD dwOldStyle = m_listCtrl.GetExtendedStyle();


	SetWindowText(L"Modules");


	m_listCtrl.SetExtendedStyle(dwOldStyle
		| LVS_EX_GRIDLINES
		| LVS_EX_FULLROWSELECT);


	m_listCtrl.InsertColumn(0, L"ModName", LVCFMT_CENTER, 350);
	m_listCtrl.InsertColumn(1, L"Base", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(2, L"Size/kB", LVCFMT_CENTER, 150);

	unsigned int nMod = 0;
	DWORD dwSize = NULL;
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_GET_MOD_NUM,
		&m_hPID,
		sizeof(m_hPID),
		&nMod,
		sizeof(nMod),
		&dwSize,
		NULL);

	PModInfo pModInfo= (PModInfo)calloc(nMod, sizeof(ModInfo));
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_ENUM_MOD,
		&m_hPID,
		sizeof(m_hPID),
		pModInfo,
		nMod * sizeof(ModInfo),
		&dwSize,
		NULL);

	PModInfo pTmp = pModInfo;
	wchar_t wszBuf[20] = { 0 };

	//	//////////////////////////////////////////
	for (unsigned int i = 0; i < nMod && (*(DWORD*)pTmp != 0); ++i, ++pTmp)
	{
		m_listCtrl.InsertItem(i, L"");

		m_listCtrl.SetItemText(i, 0, pTmp->wszModName);

		swprintf_s(wszBuf, L"0x%p", pTmp->DllBase);
		m_listCtrl.SetItemText(i, 1, wszBuf);
		memset(wszBuf, 0, sizeof(wszBuf));

		swprintf_s(wszBuf, L"%u", pTmp->SizeOfImage / 1024);
		m_listCtrl.SetItemText(i, 2, wszBuf);
		memset(wszBuf, 0, sizeof(wszBuf));

	}
	
	if(pModInfo)
	{
		free(pModInfo);
		pModInfo = NULL;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
