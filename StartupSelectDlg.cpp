#include "stdafx.h"
#include "resource.h"
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
    ON_BN_CLICKED(IDC_BTN_START_SHOPSETUP, OnBtnShopSetup)
    ON_BN_CLICKED(IDC_BTN_START_READERSETUP, OnBtnReaderSetup)
    ON_BN_CLICKED(IDC_BTN_START_KFTCONECAP, OnBtnKFTCOneCAP)
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

void CStartupSelectDlg::OnBtnKFTCOneCAP()
{
    m_choice = CHOICE_KFTC_ONECAP;
    EndDialog(IDOK);
}
