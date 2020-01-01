
// R3Dlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "R3.h"
#include "R3Dlg.h"
#include "afxdialogex.h"

#include "CDlgDriver.h"
#include "CDlgProc.h"
#include "CDlgIDT.h"
#include "CDlgGDT.h"
#include "CDlgSSDT.h"
#include "CDlgFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CR3Dlg 对话框



CR3Dlg::CR3Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_R3_DIALOG, pParent)
	, m_hServiceManager(NULL)
	, m_hService(NULL)
	, m_hDev(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CR3Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CR3Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTNDRIVER, &CR3Dlg::OnBnClickedBtndriver)
	ON_BN_CLICKED(IDC_BTNPROCESS, &CR3Dlg::OnBnClickedBtnprocess)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BTNIDT, &CR3Dlg::OnBnClickedBtnidt)
	ON_BN_CLICKED(IDC_BTNGDT, &CR3Dlg::OnBnClickedBtngdt)
	ON_BN_CLICKED(IDC_BTNSSDT, &CR3Dlg::OnBnClickedBtnssdt)
	ON_BN_CLICKED(IDC_BTNPROTECT, &CR3Dlg::OnBnClickedBtnprotect)
	ON_BN_CLICKED(IDC_BTNKERNELRELOAD, &CR3Dlg::OnBnClickedBtnkernelreload)
	ON_BN_CLICKED(IDC_BTNFILE, &CR3Dlg::OnBnClickedBtnfile)
END_MESSAGE_MAP()



void ShowError()
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		0, // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);

	MessageBoxW(NULL, (LPCTSTR)lpMsgBuf, L"Error", MB_OK | MB_ICONINFORMATION);
	// Free the buffer.
	LocalFree(lpMsgBuf);
}


// CR3Dlg 消息处理程序

BOOL CR3Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	SetWindowText(L"R3");

	memset(m_szSysPath, 0, sizeof(m_szSysPath));
	char* pTmp = _getcwd(m_szSysPath, MAX_PATH);
	if (!pTmp
		|| !PathAppendA(m_szSysPath, "\\R0.sys"))
	{
		MessageBoxW(L"PathAppendA() error", L"Error");
		ExitProcess(0);
	}

	// 1. 打开服务管理器
	m_hServiceManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	// 2. 创建服务
	m_hService = CreateServiceA(m_hServiceManager,
		"ServiceName",
		"ServiceDisplayName",
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,	//Service Type: Driver
		SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,
		m_szSysPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);
	// 2.1 若服务存在，直接打开
	if (ERROR_SERVICE_EXISTS == GetLastError())
	{
		m_hService = OpenServiceA(m_hServiceManager, "ServiceName", SERVICE_ALL_ACCESS);
	}
	// 2.2 判断是否成功
	if (!m_hService)
	{
		ShowError();
		return FALSE;
	}

	// 3. 查询服务状态，若服务暂停，则启动服务
	SERVICE_STATUS status;
	QueryServiceStatus(m_hService, &status);
	if (SERVICE_STOPPED == status.dwCurrentState)
	{
		StartServiceA(m_hService, NULL, NULL);
		Sleep(1000);

		// check again
		QueryServiceStatus(m_hService, &status);
		if (status.dwCurrentState != SERVICE_RUNNING)
		{
			ShowError();
			return FALSE;
		}
		else
		{
			UpdateData(FALSE);

		}
	}

	m_hDev = CreateFileW(
		L"\\\\.\\mysymbol",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (INVALID_HANDLE_VALUE == m_hDev)
	{
		ShowError();
	}

	MessageBoxW(L"Driver Loaded");

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CR3Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CR3Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CR3Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CR3Dlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	SERVICE_STATUS status;

	CloseHandle(m_hDev);

	QueryServiceStatus(m_hService, &status);
	if (SERVICE_STOPPED != status.dwCurrentState)
	{
		ControlService(m_hService, SERVICE_CONTROL_STOP, &status);
		if (QueryServiceStatus(m_hService, &status))
		{
			Sleep(status.dwWaitHint);
			if (SERVICE_STOPPED == status.dwCurrentState)
			{
				MessageBoxW(L"Driver Unloaded.", NULL);
			}
			else
			{
				MessageBoxW(L"Driver not Unloaded.", L"Error");
				return;
			}
		}
	}

	if (!DeleteService(m_hService))
	{
		CString cs;
		cs.Format(_T("DeleteService() Failed: %d"), GetLastError());
		MessageBox(cs, L"Error");
		return;
	}
	else
	{
		Sleep(1000);
		MessageBox(L"Service Deleted.");
	}


	CloseServiceHandle(m_hService);
	CloseServiceHandle(m_hServiceManager);

	CDialogEx::OnClose();
}



void CR3Dlg::OnBnClickedBtndriver()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgDriver* pDlgDriver = new CDlgDriver;
	pDlgDriver->SetDev(m_hDev);
	pDlgDriver->DoModal();
}


void CR3Dlg::OnBnClickedBtnprocess()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgProc* pProc = new CDlgProc;
	pProc->SetDev(m_hDev);
	pProc->DoModal();
}


void CR3Dlg::OnBnClickedBtnidt()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgIDT* pIDT = new CDlgIDT;
	pIDT->SetDev(m_hDev);
	pIDT->DoModal();
}


void CR3Dlg::OnBnClickedBtngdt()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgGDT* pGDT = new CDlgGDT;
	pGDT->SetDev(m_hDev);
	pGDT->DoModal();
}


void CR3Dlg::OnBnClickedBtnssdt()
{
	// TODO: 在此添加控件通知处理程序代码
	
	CDlgSSDT* pSSDT = new CDlgSSDT;
	pSSDT->SetDev(m_hDev);
	pSSDT->DoModal();
}


void CR3Dlg::OnBnClickedBtnprotect()
{
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwSize = 0;
	DWORD dwPID = GetCurrentProcessId();
	DeviceIoControl(
		m_hDev,
		DEVICE_CTRL_CODE_PROTECT_SELF,
		&dwPID,
		sizeof(dwPID),
		NULL,
		0,
		&dwSize,
		NULL);
}


void CR3Dlg::OnBnClickedBtnkernelreload()
{
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwSize = 0;
	DeviceIoControl(
		m_hDev, 
		DEVICE_CTRL_CODE_KERNEL_RELOAD,
		NULL,
		0,
		NULL,
		0,
		&dwSize,
		NULL);
}


void CR3Dlg::OnBnClickedBtnfile()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgFile* pDlgFile = new CDlgFile;
	pDlgFile->SetDev(m_hDev);
	pDlgFile->DoModal();
}
