#include "stdafx.h"
#include "resource.h"
#include "KFTCOneCAPDlg.h"
#include "ReaderSetupDlg.h"
#include "ShopSetupDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CKFTCOneCAPDlg::CKFTCOneCAPDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CKFTCOneCAPDlg::IDD, pParent)
{
}

void CKFTCOneCAPDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKFTCOneCAPDlg, CDialog)
    ON_BN_CLICKED(IDC_READER_SETUP, OnReaderSetup)
    ON_BN_CLICKED(IDC_SHOP_SETUP, OnShopSetup)
    ON_BN_CLICKED(IDC_TRANS, OnTrans)
    ON_BN_CLICKED(IDC_MINIMIZE, OnMinimize)
    ON_BN_CLICKED(IDC_EXIT, OnExit)
    ON_WM_CLOSE()
END_MESSAGE_MAP()

BOOL CKFTCOneCAPDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    CenterWindow();
    SetWindowText(_T("KFTC OneCAP"));
    return TRUE;
}

void CKFTCOneCAPDlg::OnReaderSetup()
{
    CReaderSetupDlg dlg(this);
    dlg.DoModal();
}

void CKFTCOneCAPDlg::OnShopSetup()
{
    CShopSetupDlg dlg(this);
    dlg.DoModal();
}

void CKFTCOneCAPDlg::OnTrans()
{
    AfxMessageBox(_T("11.0 버전에서는 KFTC OneCAP 결제 로직이 아직 연결되지 않았습니다."), MB_ICONINFORMATION);
}

void CKFTCOneCAPDlg::OnMinimize()
{
    ShowWindow(SW_MINIMIZE);
}

void CKFTCOneCAPDlg::OnExit()
{
    EndDialog(IDCANCEL);
}

void CKFTCOneCAPDlg::OnClose()
{
    EndDialog(IDCANCEL);
}
