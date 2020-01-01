#include "pch.h"
#include "CDlgGDT.h"


BOOL CDlgGDT::OnInitDialog()
{
	CDlgListBase::OnInitDialog();

	// TODO:  在此添加额外的初始化
	DWORD dwOldStyle = m_listCtrl.GetExtendedStyle();

	SetWindowText(L"GDT");

	m_listCtrl.SetExtendedStyle(dwOldStyle
		| LVS_EX_GRIDLINES
		| LVS_EX_FULLROWSELECT);


	m_listCtrl.InsertColumn(0, L"BaseAddr", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(1, L"Limit", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(2, L"Granularity", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(3, L"DPL", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(4, L"Type", LVCFMT_CENTER, 150);
	m_listCtrl.InsertColumn(5, L"Attr", LVCFMT_CENTER, 150);

	unsigned int nGdtEntry = 0;
	DWORD dwSize = 0;
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_GET_GDT_NUM,
		NULL,
		0,
		&nGdtEntry,
		sizeof(nGdtEntry),
		&dwSize,
		NULL);

	PGDTENTRY pGdtEntry = (PGDTENTRY)calloc(nGdtEntry + 1, sizeof(GDTENTRY));
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_ENUM_GDT,
		NULL,
		0,
		pGdtEntry,
		(nGdtEntry + 1 )* sizeof(GDTENTRY),
		&dwSize,
		NULL);

	CStringW strW;
	strW.Format(L"GDT(0x%p)", *(DWORD*)pGdtEntry);
	SetWindowText(strW);

	PGDTENTRY pTmp = pGdtEntry + 1;
	ULONG uData = 0;
	for (unsigned int i = 0, index = -1; i < nGdtEntry; ++i)
	{
        if (!(pTmp[i].HighWord.Bits.Pres)) continue;

		++index;
        m_listCtrl.InsertItem(index, L"");

        // BaseAddr
        uData = (ULONG)pTmp[i].BaseLow
            + ((ULONG)(pTmp[i].HighWord.Bits.BaseMid) << 16)
            + ((ULONG)(pTmp[i].HighWord.Bits.BaseHi) << 24);
        strW.Format(L"0x%p", uData);
        m_listCtrl.SetItemText(index, 0, strW);

        // Limit
        uData = pTmp[i].LimitLow
            + ((ULONG)(pTmp[i].HighWord.Bits.LimitHi) << 16);
        strW.Format(L"0x%08X", uData);
        m_listCtrl.SetItemText(index, 1, strW);

        // Granularity
        strW.Format(L"%s",
            (pTmp[i].HighWord.Bits.Granularity)
            ? L"pages"
            : L"bytes");
        m_listCtrl.SetItemText(index, 2, strW);

        // DPL
        strW.Format(L"%d", pTmp[i].HighWord.Bits.Dpl);
        m_listCtrl.SetItemText(index, 3, strW);

		// Type and Attribute
        if ((pTmp[i].HighWord.Bits.S == 0))
        {
            m_listCtrl.SetItemText(index, 4, L"System");
        }
        else
        {
            if (pTmp[i].HighWord.Bits.Type & 0x8)
            {
				m_listCtrl.SetItemText(index, 4, L"Code");

				strW.Format(L"%s%s%s",
					pTmp[i].HighWord.Bits.Type & 0x4 ? "C" : "-",
					pTmp[i].HighWord.Bits.Type & 0x2 ? "R" : "-",
					pTmp[i].HighWord.Bits.Type & 0x1 ? "A" : "-");
				m_listCtrl.SetItemText(index, 5, strW);
            }
            else
            {
				m_listCtrl.SetItemText(index, 4, L"Data");

				// Attributes
				strW.Format(L"%s%s%s",
					pTmp[i].HighWord.Bits.Type & 0x4 ? "E" : "-",
					pTmp[i].HighWord.Bits.Type & 0x2 ? "W" : "-",
					pTmp[i].HighWord.Bits.Type & 0x1 ? "A" : "-");
				m_listCtrl.SetItemText(index, 5, strW);
            }
        }

	}

	if (pGdtEntry)
	{
		free(pGdtEntry);
		pGdtEntry = NULL;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
