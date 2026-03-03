#include "stdafx.h"
#include "MerchantSetupApp.h"
#include "ShopSetupDlg.h"
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

    // Profile API (WriteProfileString) must write to:
    // HKCU\\Software\\KFTC_VAN\\KFTCOneCAP\\<SECTION>\\<FIELD>
    // -> RegistryKey = "KFTC_VAN", ProfileName = "KFTCOneCAP"
    SetRegistryKey(_T("KFTC_VAN"));
    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX) };
    icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_DATE_CLASSES;
    ::InitCommonControlsEx(&icc);
    CShopSetupDlg dlg; m_pMainWnd = &dlg; dlg.DoModal();
    return FALSE;
}
