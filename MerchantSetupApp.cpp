#include "stdafx.h"
#include "MerchantSetupApp.h"
#include "ShopSetupDlg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
CMerchantSetupApp theApp;
BOOL CMerchantSetupApp::InitInstance()
{
    CWinApp::InitInstance();
    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX) };
    icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_DATE_CLASSES;
    ::InitCommonControlsEx(&icc);
    CShopSetupDlg dlg; m_pMainWnd = &dlg; dlg.DoModal();
    return FALSE;
}
