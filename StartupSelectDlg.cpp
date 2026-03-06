// StartupSelectDlg.cpp : implementation file
//
// [v2.0 추가] 시작 시 다이얼로그 선택창

#include "stdafx.h"
#include "Resource.h"
#include "StartupSelectDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CStartupSelectDlg::CStartupSelectDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CStartupSelectDlg::IDD, pParent)
    , m_choice(CHOICE_SHOP_SETUP)
{
}

void CStartupSelectDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CStartupSelectDlg, CDialog)
    ON_BN_CLICKED(IDC_BTN_START_SHOPSETUP, &CStartupSelectDlg::OnBtnShopSetup)
    ON_BN_CLICKED(IDC_BTN_START_READERSETUP, &CStartupSelectDlg::OnBtnReaderSetup)
END_MESSAGE_MAP()

BOOL CStartupSelectDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    CenterWindow();
    return TRUE;
}

void CStartupSelectDlg::OnBtnShopSetup()
{
    m_choice = CHOICE_SHOP_SETUP;
    EndDialog(IDOK);
}

void CStartupSelectDlg::OnBtnReaderSetup()
{
    m_choice = CHOICE_READER_SETUP;
    EndDialog(IDOK);
}
