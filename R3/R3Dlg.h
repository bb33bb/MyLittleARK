
// R3Dlg.h: 头文件
//

#pragma once

#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <direct.h>
#include <winsvc.h>

// CR3Dlg 对话框
class CR3Dlg : public CDialogEx
{
// 构造
public:
	CR3Dlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_R3_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:

	HANDLE m_hDev;
	char m_szSysPath[MAX_PATH];
	SC_HANDLE m_hServiceManager;
	SC_HANDLE m_hService;

	afx_msg void OnBnClickedBtndriver();
	afx_msg void OnBnClickedBtnprocess();
	afx_msg void OnClose();
	afx_msg void OnBnClickedBtnidt();
	afx_msg void OnBnClickedBtngdt();
	afx_msg void OnBnClickedBtnssdt();
	afx_msg void OnBnClickedBtnprotect();
	afx_msg void OnBnClickedBtnkernelreload();
	afx_msg void OnBnClickedBtnfile();
};
