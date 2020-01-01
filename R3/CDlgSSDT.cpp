#include "pch.h"
#include "CDlgSSDT.h"


BOOL CDlgSSDT::OnInitDialog()
{
	CDlgListBase::OnInitDialog();

	// TODO:  在此添加额外的初始化
	DWORD dwOldStyle = m_listCtrl.GetExtendedStyle();

	SetWindowText(L"Thread");

	m_listCtrl.SetExtendedStyle(dwOldStyle
		| LVS_EX_GRIDLINES
		| LVS_EX_FULLROWSELECT);


	m_listCtrl.InsertColumn(0, L"Service Name", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(1, L"Service Func Addr", LVCFMT_CENTER, 150);


	unsigned int nService = 0;
	DWORD dwSize = NULL;
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_GET_SSDT_SERVICE_NUM,
		NULL,
		0,
		&nService,
		sizeof(nService),
		&dwSize,
		NULL);

	PULONG pSSDT = (PULONG)calloc(nService + 1, sizeof(ULONG));
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_GET_SSDT_SERVICE,
		NULL,
		0,
		pSSDT,
		(nService + 1) * sizeof(ULONG),
		&dwSize,
		NULL);

	CStringW strW;
	for (unsigned int i = 0; i < nService; ++i)
	{
		m_listCtrl.InsertItem(i, L"");

		strW.Format(L"0x%p", pSSDT[i]);
		m_listCtrl.SetItemText(i, 1, strW);
	}

	if (pSSDT)
	{
		free(pSSDT);
		pSSDT = NULL;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
