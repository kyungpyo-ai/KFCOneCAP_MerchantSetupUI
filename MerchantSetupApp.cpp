#include "stdafx.h"
#include "MerchantSetupApp.h"
#include "ShopSetupDlg.h"
#include "ModernUI.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ==============================================================
// [MerchantSetupApp.cpp]
//  - 프로그램 진입점(CWinApp 파생)
//  - 공통 컨트롤 초기화(InitCommonControlsEx)
//  - 레지스트리 저장 경로 설정(SetRegistryKey / m_pszAppName)
//  - 메인 다이얼로그(CShopSetupDlg) 실행
//
// ※ 본 프로젝트는 UI 스타일/스킨을 ModernUI.* 에 모아두고,
//    각 다이얼로그에서 해당 유틸을 사용해 커스텀 컨트롤을 구성합니다.
// ==============================================================

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

int CMerchantSetupApp::ExitInstance()
{
    ModernUIGfx::ShutdownGdiplus();
    return CWinApp::ExitInstance();
}
