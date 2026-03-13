#include "stdafx.h"
#include "MerchantSetupApp.h"
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

    ModernUIFont::EnsureFontsLoaded();

    CKFTCOneCAPDlg startDlg;
    if (startDlg.DoModal() != IDOK)
        return FALSE;

    return FALSE;
}

int CMerchantSetupApp::ExitInstance()
{
    ModernUIGfx::ShutdownGdiplus();
    return CWinApp::ExitInstance();
}
