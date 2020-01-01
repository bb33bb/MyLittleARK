// CDlgFile.cpp: 实现文件
//


#include "pch.h"
#include "R3.h"
#include "CDlgFile.h"
#include "afxdialogex.h"


// CDlgFile 对话框

IMPLEMENT_DYNAMIC(CDlgFile, CDialogEx)

CDlgFile::CDlgFile(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CDlgFile, pParent)
{

}

CDlgFile::~CDlgFile()
{
}

void CDlgFile::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCSHELLTREE1, m_tree);
	DDX_Control(pDX, IDC_LISTFILE, m_listFile);
}


BEGIN_MESSAGE_MAP(CDlgFile, CDialogEx)
	ON_NOTIFY(TVN_SELCHANGED, IDC_MFCSHELLTREE1, &CDlgFile::OnTvnSelchangedMfcshelltree1)
END_MESSAGE_MAP()


// CDlgFile 消息处理程序


void CDlgFile::OnTvnSelchangedMfcshelltree1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CStringW strW, strTmp;
	unsigned int nFile = 0;
	DWORD dwSize = 0;
	char buf[sizeof(FILE_BOTH_DIR_INFORMATION) + 266 * 2];


	m_tree.GetItemPath(strTmp, pNMTreeView->itemNew.hItem);
	strW = L"\\??\\" + strTmp;
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_GET_FILE_COUNT,
		strW.GetBuffer(),
		(strW.GetLength()+1)*2 ,
		&nFile,
		sizeof(nFile),
		&dwSize,
		NULL);

	FILE_BOTH_DIR_INFORMATION* pFileInfo = (FILE_BOTH_DIR_INFORMATION*)calloc(nFile, sizeof(buf));
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_GET_FILE_INFO,
		strW.GetBuffer(),
		(strW.GetLength() + 1) * 2,
		pFileInfo,
		nFile*sizeof(buf),
		&dwSize,
		NULL);

	FILETIME fileTime = { 0 };
	SYSTEMTIME sysTime = { 0 };
	m_listFile.DeleteAllItems();
	for (unsigned int i = 0; i < nFile; ++i)
	{
		m_listFile.InsertItem(i, L"");

		m_listFile.SetItemText(i, 0, pFileInfo->FileName);

		fileTime.dwLowDateTime = pFileInfo->CreationTime.LowPart;
		fileTime.dwHighDateTime = pFileInfo->CreationTime.HighPart;

		FileTimeToSystemTime(&fileTime, &sysTime);
		strW.Format(L"%d-%d-%d", sysTime.wYear, sysTime.wMonth, sysTime.wDay);
		m_listFile.SetItemText(i, 1, strW);

		pFileInfo = (FILE_BOTH_DIR_INFORMATION*)((unsigned int)pFileInfo + sizeof(buf));
	}

	*pResult = 0;
}

void CDlgFile::SetDev(HANDLE hDev)
{
	m_hDev = hDev;
}


BOOL CDlgFile::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	DWORD dwOldStyle = m_listFile.GetExtendedStyle();


	m_listFile.SetExtendedStyle(dwOldStyle
		| LVS_EX_GRIDLINES
		| LVS_EX_FULLROWSELECT);


	m_listFile.InsertColumn(0, L"File", LVCFMT_CENTER, 150);
	m_listFile.InsertColumn(1, L"Create", LVCFMT_CENTER, 150);


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
