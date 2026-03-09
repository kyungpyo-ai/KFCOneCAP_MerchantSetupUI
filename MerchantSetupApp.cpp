#include "stdafx.h"
#include "MerchantSetupApp.h"
#include "ShopSetupDlg.h"
#include "ReaderSetupDlg.h"
#include "StartupSelectDlg.h"
#include "KFTCOneCAPDlg.h"
#include "ModernUI.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMerchantSetupApp theApp;

CMerchantSetupApp::CMerchantSetupApp()
{
    m_pszAppName = _tcsdup(_T("KFTCOneCAP"));
}

BOOL CMerchantSetupApp::InitInstance()
{
    CWinApp::InitInstance();

    SetRegistryKey(_T("KFTC_VAN"));

    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX) };
    icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_DATE_CLASSES;
    ::InitCommonControlsEx(&icc);



    CKFTCOneCAPDlg startDlg;
    INT_PTR nStartResult = startDlg.DoModal();
    if (nStartResult != IDOK)
        return FALSE;

    INT_PTR nResult = IDCANCEL;



    UNREFERENCED_PARAMETER(nResult);
    return FALSE;
}

int CMerchantSetupApp::ExitInstance()
{
    ModernUIGfx::ShutdownGdiplus();
    return CWinApp::ExitInstance();
}
