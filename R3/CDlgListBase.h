#pragma once


// CDlgListBase 对话框

class CDlgListBase : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgListBase)

public:
	CDlgListBase(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgListBase();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CDlgListBase };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_listCtrl;
	HANDLE m_hDev;
	void SetDev(HANDLE hDev);

};
