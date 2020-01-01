// CDlgListBase.cpp: 实现文件
//

#include "pch.h"
#include "R3.h"
#include "CDlgListBase.h"
#include "afxdialogex.h"


// CDlgListBase 对话框

IMPLEMENT_DYNAMIC(CDlgListBase, CDialogEx)

CDlgListBase::CDlgListBase(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CDlgListBase, pParent)
	, m_hDev(NULL)
{

}

CDlgListBase::~CDlgListBase()
{
}

void CDlgListBase::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTBASE, m_listCtrl);
}

void CDlgListBase::SetDev(HANDLE hDev)
{
	m_hDev = hDev;
}


BEGIN_MESSAGE_MAP(CDlgListBase, CDialogEx)
END_MESSAGE_MAP()


// CDlgListBase 消息处理程序
