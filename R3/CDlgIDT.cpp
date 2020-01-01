#include "pch.h"
#include "CDlgIDT.h"


BOOL CDlgIDT::OnInitDialog()
{
	CDlgListBase::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetWindowText(L"IDT");

	DWORD dwOldStyle = m_listCtrl.GetExtendedStyle();

	m_listCtrl.SetExtendedStyle(dwOldStyle | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	m_listCtrl.InsertColumn(0, L"Interrupted Num", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(1, L"Interrupted Addr", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(2, L"Selector", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(3, L"GateType", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(4, L"DPL", LVCFMT_CENTER, 150);

	unsigned int nIdtEntry = 0;
	// The first one is IDTR
	PIDTENTRY pIdtEntry = (PIDTENTRY)calloc(257, sizeof(IDTENTRY));

	DWORD dwSize = 0;
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_IDT,
		NULL,
		0,
		pIdtEntry,
		257 * sizeof(IDTENTRY),
		&dwSize,
		NULL);

	CStringW strW;
	strW.Format(L"IDT(0x%p)", *(DWORD*)pIdtEntry);
	SetWindowText(strW);

	PIDTENTRY pTmp = pIdtEntry + 1;
	for (unsigned int i = 0; i < 256; ++i, ++pTmp)
	{
		m_listCtrl.InsertItem(i, L"");

		strW.Format(L"%d", i);
		m_listCtrl.SetItemText(i, 0, strW);
		strW.Format(L"0x%p", (pTmp->uOffsetLow) + (pTmp->uOffsetHigh << 16));
		m_listCtrl.SetItemText(i, 1, strW);
		strW.Format(L"%d", pTmp->uSelector);
		m_listCtrl.SetItemText(i, 2, strW);
		strW.Format(L"%d", pTmp->GateType);
		m_listCtrl.SetItemText(i, 3, strW);
		strW.Format(L"%d", pTmp->DPL);
		m_listCtrl.SetItemText(i, 4, strW);
	}

	if (pIdtEntry)
	{
		free(pIdtEntry);
		pIdtEntry = NULL;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
