#include "stdafx.h"
#include "Resource.h"
#include "ShopSetupDlg.h"

#include "ModernMessageBox.h"
#include "ShopDownDlg.h"
#include "ModernUI.h"
#include "RegistryUtil.h"
#include <gdiplus.h>
// Forward declarations (defined in common.cpp)
BOOL LaunchExeInSameDir(LPCTSTR exeName);
BOOL TerminateExeByName(LPCTSTR exeName, DWORD gracePeriodMs = 3000);
#pragma comment(lib, "gdiplus.lib")
// ==============================================================
// [ShopSetupDlg.cpp]
//  - 메인 설정 다이얼로그 구현부
//
// 화면 동작 개요
//  1) OnInitDialog()
//     - 폰트/브러시/ModernUI 컨트롤 초기화
//     - 탭/그룹/인풋/토글/버튼 생성
//     - ApplyLayout()으로 DPI 반영 배치
//     - LoadOptionsFromRegistry()로 저장값을 UI에 반영
//  2) 사용자 입력(콤보/에딧/토글) → 멤버변수/컨트롤 상태 갱신
//  3) OnOK() 또는 적용 버튼에서 SaveOptionsToRegistry() 호출
//     - 현재 UI 값을 레지스트리(HKCU\Software\KFTC_VAN\KFTCOneCAP\...)에 저장
//
// 유지보수 팁
//  - 옵션 추가 시: (1) 컨트롤 생성 (2) Load/Save 매핑 (3) 레이아웃 반영 순서로 작업
//  - 인풋 보더/포커스 스타일은 ModernUITheme::GetInputTheme() 기준
// ==============================================================
// ============================================================================
// Registry spec (Word 기준)
// - 저장: AfxGetApp()->WriteProfileString(section, field, value) 그대로 사용
// - 불러오기: GetRegisterData(section, field, outValue) 로만 접근
// ============================================================================
// ============================================================================
// [검증 프로세스 유지보수 안내]
// ----------------------------------------------------------------------------
// 1) 실시간 검증
//    - 각 EditText의 EN_CHANGE가 발생하면 OnEnChangeValidateInput()이 호출된다.
//    - 여기서 ValidateControlAndUpdateUI(ctrlId)를 통해 "현재 입력 중인 Edit 1개만" 검사한다.
//    - 부모 다이얼로그 전체를 다시 그리지 않고 오류문구 Static / Edit 오류상태만 최소 범위로 갱신한다.
//
// 2) 확인 버튼 검증
//    - OnOK()에서는 ValidateAllInputs(FALSE)로 먼저 조용히 전체 검사를 수행한다.
//    - 오류가 있으면 알림창을 띄운 뒤, 해당 탭으로 이동하고 ValidateAllInputs(TRUE)로 오류문구를 표시한다.
//    - 첫 번째 오류 EditText로 포커스를 이동시키고, 오류가 없을 때만 SaveOptionsToRegistry()를 호출한다.
//
// 3) 조건부 검증 / 비활성화 제외 규칙
//    - 우선 거래 프로그램: 카드 감지 우선 거래 사용 ON일 때만 검증
//    - 스캐너 포트번호 : 스캐너 사용 ON일 때만 검증
//    - 서명패드 포트번호: 서명패드 사용여부가 예일 때만 검증
//    - 즉, EnableWindow(FALSE) 상태가 된 Edit는 검증 대상에서 제외되어야 한다.
//    - 이 규칙은 ValidateSingleField()와 UpdateToggleDependentEdits()에서 같이 보장한다.
//
// 4) 오류 표시 방식
//    - 오류문구는 Edit 오른쪽 Static으로 표시하며 빨간색 + 굵은 글씨를 사용한다.
//    - 오류 Edit는 포커스가 있을 때 빨간 테두리로 표시한다.
//    - 다른 탭의 오류문구가 보이지 않도록 RefreshValidationVisibilityByTab()가 현재 탭 기준으로 정리한다.
//
// 5) 안정성 가드
//    - m_bUiInitialized : UI 생성 완료 전 이벤트 진입 방지
//    - m_bClosing       : 종료 중 / 파괴 후 컨트롤 접근 방지
//    - 검증/오류표시/탭전환 관련 함수는 두 플래그를 먼저 확인하고 조기 리턴한다.
//
// [초기값(레지스트리 값이 없을 때)]
//    - 실제 초기값 지정 위치는 LoadOptionsFromRegistry() 내부의 각 if (GetRegisterData(...)) else 블록이다.
//    - 예) 포트번호 8002, 세금 자동 역산 0, 카드입력 Timeout 100, 무서명 기준 금액 50000
//    - 예) 서명패드 포트번호 0, 스캐너 포트번호 0, 우선 거래 프로그램 공백 문자열
//    - 토글/콤보 기본값은 ReadToggle_DefaultOnWhenMissing() 및 SelectComboByValue(..., defaultValue)에서 설정한다.
// ============================================================================
namespace
{
    // Sections
    static LPCTSTR SEC_TCP = _T("TCP");
    static LPCTSTR SEC_SERIALPORT = _T("SERIALPORT");
    // TCP
    static LPCTSTR VAN_SERVER_IP_FIELD = _T("VAN_SERVER_IP");
    static LPCTSTR VAN_SERVER_PORT_FIELD = _T("VAN_SERVER_PORT");
    static LPCTSTR TAX_SETTING_FIELD = _T("TAX_SETTING");   // 세금 자동 역산 (IDC_EDIT_TAX_PERCENT)
    // SERIALPORT
    static LPCTSTR TIMEOUT_FIELD = _T("TIMEOUT");
    static LPCTSTR NOSIGN_AMT_FIELD = _T("NOSIGN_AMT");
    static LPCTSTR CASH_FIRST_FIELD = _T("CASH_FIRST");
    static LPCTSTR INTERLOCK_FIELD = _T("INTERLOCK");
    static LPCTSTR SOCKET_TYPE_FIELD = _T("SOCKET_TYPE");
    static LPCTSTR SIGNPAD_USE_FIELD = _T("SIGNPAD_USE");
    static LPCTSTR SIGNPAD_FIELD = _T("SIGNPAD");
    static LPCTSTR SIGNPAD_SPEED_FIELD = _T("SIGNPAD_SPEED");
    static LPCTSTR NOTIFY_POS_FIELD = _T("NOTIFY_POS");
    static LPCTSTR NOTIFY_SIZE_FIELD = _T("NOTIFY_SIZE");
    static LPCTSTR CANCEL_HOTKEY_FIELD = _T("CANCEL_HOTKEY");
    static LPCTSTR MSR_HOTKEY_FIELD = _T("MSR_HOTKEY");
    static LPCTSTR MULTIPAD_SOUND_FIELD = _T("MULTIPAD_SOUND");
    static LPCTSTR BARCODE_USE_FIELD = _T("BARCODE_USE");
    static LPCTSTR BARCODE_PORT_FIELD = _T("BARCODE_PORT");
    static LPCTSTR CARD_DETECT_FIELD = _T("CARD_DETECT");
    static LPCTSTR DETECT_PROGRAM_FIELD = _T("DETECT_PROGRAM");
    static LPCTSTR AUTO_REBOOT_FIELD = _T("AUTO_REBOOT");
    static LPCTSTR NOTIFY_IMG_FIELD = _T("NOTIFY_IMG");
    static LPCTSTR NOTIFY_DUAL_FIELD = _T("NOTIFY_DUAL_MONITOR");
    struct ComboItem
    {
        LPCTSTR text;   // 화면 표시값
        LPCTSTR value;  // 저장값(레지스트리 데이터)
    };
    static void FillCombo(CSkinnedComboBox& cb, const ComboItem* items, int count)
    {
        cb.ResetContent();
        for (int i = 0; i < count; ++i)
            cb.AddString(items[i].text);
        cb.SetCurSel(0);
    }
    static int FindIndexByValue(const ComboItem* items, int count, const CString& value)
    {
        for (int i = 0; i < count; ++i)
        {
            if (value.CompareNoCase(items[i].value) == 0)
                return i;
        }
        return -1;
    }
    static void SelectComboByValue(CSkinnedComboBox& cb, const ComboItem* items, int count,
        const CString& value, int defaultIndex)
    {
        int idx = FindIndexByValue(items, count, value);
        if (idx < 0) idx = defaultIndex;
        if (idx < 0) idx = 0;
        if (idx >= count) idx = 0;
        cb.SetCurSel(idx);
    }
    static CString GetSelectedComboValue(const CSkinnedComboBox& cb, const ComboItem* items, int count,
        LPCTSTR defaultValue)
    {
        int idx = cb.GetCurSel();
        if (idx < 0 || idx >= count)
            return CString(defaultValue ? defaultValue : _T(""));
        return CString(items[idx].value);
    }
    // Combo mappings (Word 기준)
    static const ComboItem kVanServers[] =
    {
        { _T("운영 서버(www.kftcvan.or.kr)"), _T("www.kftcvan.or.kr") },
        { _T("테스트 서버"),                 _T("203.175.190.145") },
        { _T("테스트 서버(내부용)"),         _T("192.168.53.28") },
    };
    static const ComboItem kCashReceipt[] =
    {
        { _T("PINPAD/KEYIN"), _T("PINPAD/KEYIN") },
        { _T("MS"),           _T("MS") },
        { _T("KEYIN"),        _T("KEYIN") },
    };
    static const ComboItem kInterlock[] =
    {
        { _T("IC/MS 리더기"),          _T("NORMAL") },
        { _T("멀티패드(동반위)"),      _T("TRANSINFO") },
        { _T("AOP(NpayConnect)"),  _T("AOP") },
        { _T("단말기(forPOS)"),        _T("FORPOS") },
        { _T("LockType 리더기"),       _T("LOCKTYPE(TDR)") },
        { _T("AutoDriven 리더기"),     _T("LOCKTYPE(TTM)") },
        { _T("멀티패드(복지단)"),      _T("DP636-MND") },
        { _T("연동 안함"),             _T("NOTHING") },
    };
    static const ComboItem kCommType[] =
    {
        { _T("CS 방식"),  _T("CS 방식") },
        { _T("WEB 방식"), _T("WEB 방식") },
    };
    static const ComboItem kSignPadUse[] =
    {
        { _T("예"),       _T("YES") },
        { _T("아니오"),   _T("NO") },
        { _T("자체 서명"), _T("SELF") },
    };
    static const ComboItem kSignPadSpeed[] =
    {
        { _T("57600bps"),  _T("57600") },
        { _T("115200bps"), _T("115200") },
    };
    static const ComboItem kAlarmPos[] =
    {
        { _T("기본"),      _T("default") },
        { _T("중앙"),      _T("mid") },
        { _T("표시 안함"), _T("hide") },
    };
    static const ComboItem kAlarmSize[] =
    {
        { _T("기본"),     _T("default") },
        { _T("매우작게"), _T("verysmall") },
        { _T("작게"),     _T("small") },
        { _T("크게"),     _T("big") },
        { _T("매우크게"), _T("very big") },
    };
    static const ComboItem kHotkeys[] =
    {
        { _T("기본"),      _T("NORMAL") },
        { _T("F1"),        _T("VK_F1") },   { _T("F2"), _T("VK_F2") },   { _T("F3"), _T("VK_F3") },   { _T("F4"), _T("VK_F4") },
        { _T("F5"),        _T("VK_F5") },   { _T("F6"), _T("VK_F6") },   { _T("F7"), _T("VK_F7") },   { _T("F8"), _T("VK_F8") },
        { _T("F9"),        _T("VK_F9") },   { _T("F10"), _T("VK_F10") }, { _T("F11"), _T("VK_F11") }, { _T("F12"), _T("VK_F12") },
        { _T("ESC"),       _T("VK_ESCAPE") },
        { _T("ENTER"),     _T("VK_RETURN") },
        { _T("SPACE"),     _T("VK_SPACE") },
        { _T("TAB"),       _T("VK_TAB") },
        { _T("BACKSPACE"), _T("VK_BACK") },
        { _T("INSERT"),    _T("VK_INSERT") },
        { _T("DELETE"),    _T("VK_DELETE") },
        { _T("HOME"),      _T("VK_HOME") },
        { _T("END"),       _T("VK_END") },
        { _T("PAGEUP"),    _T("VK_PRIOR") },
        { _T("PAGEDOWN"),  _T("VK_NEXT") },
        { _T("UP"),        _T("VK_UP") },
        { _T("DOWN"),      _T("VK_DOWN") },
        { _T("LEFT"),      _T("VK_LEFT") },
        { _T("RIGHT"),     _T("VK_RIGHT") },
    };
    // Toggle mapping helpers
    static BOOL ReadToggle_DefaultOnWhenMissing(LPCTSTR field, BOOL bDefaultOn, LPCTSTR valueOn, LPCTSTR valueOff)
    {
        CString s;
        if (!GetRegisterData(SEC_SERIALPORT, field, s))
            return bDefaultOn;
        if (s.CompareNoCase(valueOn) == 0) return TRUE;
        if (s.CompareNoCase(valueOff) == 0) return FALSE;
        return bDefaultOn;
    }
    static void WriteToggleValue(LPCTSTR field, BOOL bOn, LPCTSTR valueOn, LPCTSTR valueOff)
    {
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, field, bOn ? valueOn : valueOff);
    }

    // ----------------------------------------------------------------
} // namespace
// ============================================================================
// [TUNE] 헤더 / 탭 / 컨텐츠 레이아웃 튜닝 파라미터
// ============================================================================
// ── 헤더 영역 ────────────────────────────────────────────────────────────────
// [TUNE] kHdrBadgeY    : 배지 아이콘 상단 Y (다이얼로그 좌상단 기준)
static const int kHdrBadgeY = 28;   // [TUNE] 배지 상단 위치 (기본 20)
// [TUNE] kHdrBadgeSz   : 배지 크기(정사각형)
static const int kHdrBadgeSz = 44;   // [TUNE] 배지 크기 px (기본 38)
// [TUNE] kHdrBadgeX    : 배지 좌측 여백
static const int kHdrBadgeX = 34;   // [TUNE] 배지 좌측 여백 (기본 26)
// [TUNE] kHdrTitleGap  : 배지-텍스트 간격
static const int kHdrTitleGap = 13;   // [TUNE] 배지→텍스트 간격 (기본 13)
// [TUNE] kHdrDividerY  : 헤더 하단 구분선 Y 위치
static const int kHdrDividerY = 84;   // [TUNE] 헤더 구분선 Y (기본 76)
// ── 탭 컨트롤 ────────────────────────────────────────────────────────────────
// [TUNE] kTabBarTop : 탭 바 상단 Y (헤더 구분선 아래)
static const int kTabBarTop = kHdrDividerY + 6;  // [TUNE] 탭 바 시작 Y
static const int kTabBarH = 34;                // [TUNE] 탭 바 높이
static const int kTabPadTop = 12;                // [TUNE] 탭 내용 상단 여백
static const int kTabPadLeft = 40;                // [TUNE] 탭 내용 좌측 여백
// [DEPRECATED] kHeaderShiftY - 하위 호환용 (새 코드는 kHdrDividerY 사용)
static const int kHeaderShiftY = kHdrDividerY - 88; // 자동 계산
// 컨텐츠 시작 Y = kTabBarTop + kTabBarH + kTabPadTop
static const int kContentStartY = kTabBarTop + kTabBarH + kTabPadTop;
// ── 그룹/카드 공통 ───────────────────────────────────────────────────────────
static const int kGroupTitleH = 20;
static const int kGroupGapBelowTitle = 1;
static const int kGapToNextGroup = 6;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
IMPLEMENT_DYNAMIC(CShopSetupDlg, CDialog)
BEGIN_MESSAGE_MAP(CShopSetupDlg, CDialog)
    ON_WM_DRAWITEM()
    ON_WM_MEASUREITEM()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_DESTROY()
    ON_WM_CTLCOLOR()
    ON_WM_LBUTTONDOWN()
    ON_WM_TIMER()
    ON_WM_NCACTIVATE()          // [FIX v2.1] xxxSaveDlgFocus O(N^2) 차단
    ON_WM_ACTIVATE()
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, OnTcnSelchange)
    ON_COMMAND_RANGE(IDC_BTN_VAN_SERVER_INFO, IDC_BTN_SIGN_PAD_PORT_INFO, OnInfoButtonClicked)
    ON_CBN_SELCHANGE(IDC_COMBO_SIGN_PAD_USE, OnCbnSelchangeSignPadUse)
    ON_BN_CLICKED(IDC_CHECK_CARD_DETECT, OnBnClickedCardDetectToggle)
    ON_BN_CLICKED(IDC_CHECK_SCANNER_USE, OnBnClickedScannerUseToggle)
    ON_EN_CHANGE(IDC_EDIT_PORT, OnEnChangeValidateInput)
    ON_EN_CHANGE(IDC_EDIT_NO_SIGN_AMOUNT, OnEnChangeValidateInput)
    ON_EN_CHANGE(IDC_EDIT_TAX_PERCENT, OnEnChangeValidateInput)
    ON_EN_CHANGE(IDC_EDIT_CARD_TIMEOUT, OnEnChangeValidateInput)
    ON_EN_CHANGE(IDC_EDIT_CARD_DETECT_PARAM, OnEnChangeValidateInput)
    ON_EN_CHANGE(IDC_EDIT_SIGN_PAD_PORT, OnEnChangeValidateInput)
    ON_EN_CHANGE(IDC_EDIT_SCANNER_PORT, OnEnChangeValidateInput)
END_MESSAGE_MAP()
// +++ 추가: 시스템 기본 메시지 처리로 변경
BOOL CShopSetupDlg::OnNcActivate(BOOL bActive)
{
    return (BOOL)::DefWindowProc(m_hWnd, WM_NCACTIVATE, bActive, 0);
}
void CShopSetupDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    ::DefWindowProc(m_hWnd, WM_ACTIVATE, nState, (LPARAM)(pWndOther ? pWndOther->m_hWnd : NULL));
}
// ============================================================================
// 생성자 / 소멸자
// ============================================================================
CShopSetupDlg::CShopSetupDlg(CWnd* pParent)
    : CDialog(IDD_SHOP_SETUP_DLG, pParent)
    , m_nActiveTab(0)
    , m_uHoverTimer(0)
    , m_nHoverInputId(-1)
    , m_bUiInitialized(FALSE)
    , m_bClosing(FALSE)
{
    m_intPort = 8002;
    m_intCardTimeout = 60;   // 생성자 초기값 (레지스트리 없을 때 기본값 100 적용)
    m_intNoSignAmount = 50000;
    m_intTaxPercent = 10;
    m_strCardDetectParam = _T("KFTCOneCAP TEST");
    m_intSignPadPort = 56;
    m_intScannerPort = 0;
    m_bMerchantDownloaded = FALSE;
    m_hFontCardTitle = nullptr;
    m_hFontHdrTitle = nullptr;
    m_hFontHdrSub = nullptr;
}
CShopSetupDlg::~CShopSetupDlg()
{
    if (m_fontTitle.GetSafeHandle())      m_fontTitle.DeleteObject();
    if (m_fontSubtitle.GetSafeHandle())   m_fontSubtitle.DeleteObject();
    if (m_fontSection.GetSafeHandle())    m_fontSection.DeleteObject();
    if (m_fontLabel.GetSafeHandle())      m_fontLabel.DeleteObject();
    if (m_fontGroupTitle.GetSafeHandle()) m_fontGroupTitle.DeleteObject();
    if (m_fontValidation.GetSafeHandle()) m_fontValidation.DeleteObject();
    if (m_brushBg.GetSafeHandle())        m_brushBg.DeleteObject();
    if (m_brushWhite.GetSafeHandle())     m_brushWhite.DeleteObject();
    if (m_brushTabContent.GetSafeHandle()) m_brushTabContent.DeleteObject();
    if (m_brushSection.GetSafeHandle())   m_brushSection.DeleteObject();
    if (m_hFontCardTitle) { ::DeleteObject(m_hFontCardTitle); m_hFontCardTitle = nullptr; }
    if (m_hFontHdrTitle)  { ::DeleteObject(m_hFontHdrTitle);  m_hFontHdrTitle  = nullptr; }
    if (m_hFontHdrSub)    { ::DeleteObject(m_hFontHdrSub);    m_hFontHdrSub    = nullptr; }
}
// ============================================================================
// Amount field helpers: parse comma-formatted text, format int with commas
static int ParseAmountText(const CString& s)
{
    CString digits;
    for (int i = 0; i < s.GetLength(); ++i)
        if (_istdigit(s[i])) digits += s[i];
    return _ttoi(digits);
}
static CString FormatAmountWithCommas(int n)
{
    if (n < 0) n = 0;
    CString s; s.Format(_T("%d"), n);
    int len = s.GetLength();
    CString result;
    for (int i = 0; i < len; ++i)
    {
        result += s[i];
        int rem = len - 1 - i;
        if (rem > 0 && rem % 3 == 0) result += _T(',');
    }
    return result;
}
// DoDataExchange
// ============================================================================
void CShopSetupDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_VAN_SERVER, m_comboVanServer);
    DDX_Control(pDX, IDC_EDIT_PORT, m_editPort);
    DDX_Control(pDX, IDC_EDIT_NO_SIGN_AMOUNT, m_editNoSignAmount);
    DDX_Control(pDX, IDC_EDIT_TAX_PERCENT, m_editTaxPercent);
    DDX_Control(pDX, IDC_EDIT_CARD_TIMEOUT, m_editCardTimeout);
    DDX_Control(pDX, IDC_EDIT_CARD_DETECT_PARAM, m_editCardDetectParam);
    DDX_Control(pDX, IDC_EDIT_SIGN_PAD_PORT, m_editSignPadPort);
    DDX_Control(pDX, IDC_EDIT_SCANNER_PORT, m_editScannerPort);
    { CString _s; if (!pDX->m_bSaveAndValidate) _s.Format(_T("%d"), m_intPort);        DDX_Text(pDX, IDC_EDIT_PORT, _s); if (pDX->m_bSaveAndValidate) m_intPort = _ttoi(_s); }
    { CString _s; if (!pDX->m_bSaveAndValidate) _s.Format(_T("%d"), m_intCardTimeout);  DDX_Text(pDX, IDC_EDIT_CARD_TIMEOUT, _s); if (pDX->m_bSaveAndValidate) m_intCardTimeout = _ttoi(_s); }
    { CString _s; if (!pDX->m_bSaveAndValidate) _s = FormatAmountWithCommas(m_intNoSignAmount); DDX_Text(pDX, IDC_EDIT_NO_SIGN_AMOUNT, _s); if (pDX->m_bSaveAndValidate) m_intNoSignAmount = ParseAmountText(_s); }
    { CString _s; if (!pDX->m_bSaveAndValidate) _s.Format(_T("%d"), m_intTaxPercent);   DDX_Text(pDX, IDC_EDIT_TAX_PERCENT, _s); if (pDX->m_bSaveAndValidate) m_intTaxPercent = _ttoi(_s); }
    DDX_Control(pDX, IDC_COMBO_CASH_RECEIPT, m_comboCashReceipt);
    DDX_Control(pDX, IDC_COMBO_INTERLOCK, m_comboInterlock);
    DDX_Control(pDX, IDC_COMBO_COMM_TYPE, m_comboCommType);
    DDX_Text(pDX, IDC_EDIT_CARD_DETECT_PARAM, m_strCardDetectParam);
    DDX_Control(pDX, IDC_COMBO_SIGN_PAD_USE, m_comboSignPadUse);
    { CString _s; if (!pDX->m_bSaveAndValidate) _s.Format(_T("%d"), m_intSignPadPort);  DDX_Text(pDX, IDC_EDIT_SIGN_PAD_PORT, _s); if (pDX->m_bSaveAndValidate) m_intSignPadPort = _ttoi(_s); }
    DDX_Control(pDX, IDC_COMBO_SIGN_PAD_SPEED, m_comboSignPadSpeed);
    { CString _s; if (!pDX->m_bSaveAndValidate) _s.Format(_T("%d"), m_intScannerPort);  DDX_Text(pDX, IDC_EDIT_SCANNER_PORT, _s); if (pDX->m_bSaveAndValidate) m_intScannerPort = _ttoi(_s); }
    DDX_Control(pDX, IDC_COMBO_ALARM_POS, m_comboAlarmPos);
    DDX_Control(pDX, IDC_COMBO_ALARM_SIZE, m_comboAlarmSize);
    DDX_Control(pDX, IDC_COMBO_CANCEL_KEY, m_comboCancelKey);
    DDX_Control(pDX, IDC_COMBO_MSR_KEY, m_comboMSRKey);
}
// ============================================================================
// Edit 높이를 콤보 높이로 통일
// ============================================================================
static void NormalizeInputHeightsToCombo(CWnd* pDlg, int comboId,
    const int* editIds, int editCount)
{
    if (!pDlg) return;
    CWnd* pCombo = pDlg->GetDlgItem(comboId);
    if (!pCombo || !::IsWindow(pCombo->GetSafeHwnd())) return;
    CRect rcCombo;
    pCombo->GetWindowRect(&rcCombo);
    // Use the combo's actual closed window height as the target height for edits
    int h = rcCombo.Height();
    if (h < 22) h = 22;
    for (int i = 0; i < editCount; ++i)
    {
        CWnd* pEdit = pDlg->GetDlgItem(editIds[i]);
        if (!pEdit || !::IsWindow(pEdit->GetSafeHwnd())) continue;
        CRect rc;
        pEdit->GetWindowRect(&rc);
        pDlg->ScreenToClient(&rc);
        pEdit->SetWindowPos(NULL, rc.left, rc.top, rc.Width(), h,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
// ============================================================================
// OnInitDialog
// ============================================================================
// --------------------------------------------------------------
// 다이얼로그 초기화
//  - 컨트롤 생성/스타일 적용/기본값 세팅/레지스트리 로드까지 한 번에 수행
// --------------------------------------------------------------
BOOL CShopSetupDlg::OnInitDialog()
{
    /* [UI-STEP] 초기 UI 구성(컨트롤 생성/폰트/레이아웃/값 로드) 흐름
     * 1) 다이얼로그 기본 초기화(베이스 클래스 처리) 후, 커스텀 컨트롤/리소스를 준비한다.
     * 2) EnsureFonts()로 라벨/본문/캡션 폰트를 생성하고 컨트롤에 적용한다.
     * 3) InitializeControls()에서 탭/입력/토글/버튼/정보아이콘(i) 등 실제 컨트롤을 생성/연결한다.
     * 4) ApplyLayout()로 현재 DPI와 창 크기에 맞춰 좌표를 계산하고 컨트롤을 재배치한다.
     * 5) LoadOptionsFromRegistry()로 레지스트리 값을 읽어 UI(콤보 선택/에딧 텍스트/토글 상태)에 반영한다.
     * 6) 첫 화면 그리기 품질을 위해 Invalidate()/UpdateWindow() 호출 여부를 정리한다.
     *
     * [참고]
     * - 초기화 순서가 바뀌면(예: 레지스트리 로드가 레이아웃보다 먼저) 컨트롤 크기/상태가 어긋날 수 있다.
     * - 컨트롤 생성은 1회, 배치는 여러 번(리사이즈/탭 전환) 호출되도록 분리해두는 것이 유지보수에 유리하다.
     */
    CDialog::OnInitDialog();
    // +++ 추가: 시스템 레벨 더블 버퍼링 및 자식 클리핑 적용
    //ModifyStyleEx(0, WS_EX_COMPOSITED);
    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    // --------------------------------------------------------
    // [v2.0] 시작 시 다이얼로그 선택(가맹점 설정 / 리더기 설정)
    //  - 기존에는 프로그램 실행 시 바로 CShopSetupDlg가 열렸으나,
    //    이제는 선택 창에서 어떤 설정 화면을 열지 결정한다.
    //  - 선택 창은 모달로 띄워서, CShopSetupDlg가 화면에 표시되기 전에
    //    사용자가 먼저 선택할 수 있도록 한다.
    // --------------------------------------------------------

    m_brushBg.CreateSolidBrush(RGB(249, 250, 252));  // 밝은 회색 배경
    m_brushWhite.CreateSolidBrush(RGB(255, 255, 255));
    m_brushTabContent.CreateSolidBrush(RGB(255, 255, 255));  // card white
    EnsureFonts();
    ModernUIGfx::EnsureGdiplusStartup();
    {
        LOGFONT lf = {};
        lf.lfCharSet = DEFAULT_CHARSET;
        ModernUIFont::ApplyUIFontFace(lf);
        lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 15);
        lf.lfWeight = FW_BOLD;
        m_hFontCardTitle = ::CreateFontIndirect(&lf);
        lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 18);
        m_hFontHdrTitle = ::CreateFontIndirect(&lf);
        lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 13);
        lf.lfWeight = FW_NORMAL;
        m_hFontHdrSub = ::CreateFontIndirect(&lf);
    }




    // --------------------------------------------------------
    // 탭 컨트롤 생성 (다이얼로그 리소스에 없으므로 동적 생성)
    // --------------------------------------------------------
    m_tabCtrl.Create(this, IDC_TAB_MAIN, CRect(0, 0, 10, 10));

    m_tabCtrl.SetFont(&m_fontLabel);
    // Info icon buttons
    auto CreateInfoBtn = [&](CInfoIconButton& btn, UINT id) {
        btn.Create(_T(""), WS_CHILD | BS_OWNERDRAW,
            CRect(0, 0, SX(22), SX(22)), this, id);
        };
    CreateInfoBtn(m_btnVanInfo, IDC_BTN_VAN_SERVER_INFO);
    CreateInfoBtn(m_btnPortInfo, IDC_BTN_PORT_INFO);
    CreateInfoBtn(m_btnTaxPercentInfo, IDC_BTN_TAX_PERCENT_INFO);
    CreateInfoBtn(m_btnCommTypeInfo, IDC_BTN_COMM_TYPE_INFO);
    CreateInfoBtn(m_btnCashReceiptInfo, IDC_BTN_CASH_RECEIPT_INFO);
    CreateInfoBtn(m_btnCardTimeoutInfo, IDC_BTN_CARD_TIMEOUT_INFO);
    CreateInfoBtn(m_btnInterlockInfo, IDC_BTN_INTERLOCK_INFO);
    CreateInfoBtn(m_btnMultiVoiceInfo, IDC_BTN_MULTI_VOICE_INFO);
    CreateInfoBtn(m_btnCardDetectInfo, IDC_BTN_CARD_DETECT_INFO);
    CreateInfoBtn(m_btnScannerUseInfo, IDC_BTN_SCANNER_USE_INFO);
    CreateInfoBtn(m_btnAutoRebootInfo, IDC_BTN_AUTO_REBOOT_INFO);
    CreateInfoBtn(m_btnAlarmGraphInfo, IDC_BTN_ALARM_GRAPH_INFO);
    CreateInfoBtn(m_btnAlarmDualInfo, IDC_BTN_ALARM_DUAL_INFO);
    CreateInfoBtn(m_btnSignPadUseInfo, IDC_BTN_SIGN_PAD_USE_INFO);
    CreateInfoBtn(m_btnSignPadPortInfo, IDC_BTN_SIGN_PAD_PORT_INFO);
    CreateInfoBtn(m_btnSignPadSpeedInfo, IDC_BTN_SIGN_PAD_SPEED_INFO);
    CreateInfoBtn(m_btnAlarmSizeInfo, IDC_BTN_ALARM_SIZE_INFO);
    m_tabCtrl.AddTab(_T("결제 설정"), 0);
    m_tabCtrl.AddTab(_T("장치 정보"), 1);
    m_tabCtrl.AddTab(_T("시스템 설정"), 2);
    m_tabCtrl.AddTab(_T("가맹점 다운로드"), 3);
    InitializeControls();

    // [UI 개선] 탭 전환 시 딜레이 방지를 위해 자식 다이얼로그를 미리 생성해 숨겨둡니다.
    if (m_staticShopContainer.GetSafeHwnd() && !m_shopDownDlg.GetSafeHwnd()) {
        m_shopDownDlg.Create(CShopDownDlg::IDD, &m_staticShopContainer);
        m_shopDownDlg.ShowWindow(SW_HIDE);
    }
    EnsureValidationStatics();
    LoadOptionsFromRegistry();
    // 다이얼로그 크기
    const int MARGIN_X = SX(kTabPadLeft);
    const int LABEL_W = SX(92);
    const int FIELD_W = SX(120);
    const int COL_GAP = SX(16);
    // [TUNE] 가맹점 다운로드 컬럼 폭 (합계가 kDialogMinW 이내여야 함)
    const int sd_padX = SX(10);   // [TUNE] 좌우 여백
    const int sd_gap = SX(8);    // [TUNE] 컬럼 간격
    const int sd_tagW = SX(60);   // [TUNE] 가맹점N 태그 폭
    const int sd_prodW = SX(105);  // [TUNE] 단말기 제품번호
    const int sd_bizW = SX(105);  // [TUNE] 사업자번호
    const int sd_pwdW = SX(48);   // [TUNE] 비밀번호
    const int sd_btnW = SX(82);   // [TUNE] 다운로드 버튼
    const int sd_etcW = SX(80);   // [TUNE] 단말기별 가맹점
    const int sd_nameW = SX(110);  // [TUNE] 대표 가맹점
    int topContentW = (LABEL_W + FIELD_W) * 3 + (COL_GAP * 2);
    int topMinW = (MARGIN_X * 2) + topContentW;
    int shopInnerW = (sd_padX * 2) + sd_tagW + sd_prodW + sd_bizW
        + sd_pwdW + sd_btnW + sd_etcW + sd_nameW + (sd_gap * 6);
    int bottomMinW = shopInnerW + 2 * MARGIN_X;
    const int kDialogMinW = 760;  // [TUNE] 다이얼로그 최소폭
    int dialogWidth = max(kDialogMinW, max(topMinW, bottomMinW));
    int dialogHeight = CalculateRequiredHeight();
    SetWindowPos(NULL, 0, 0, dialogWidth, dialogHeight,
        SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();
    ApplyLayout();
    // 첫 번째 탭 표시
    m_tabCtrl.SetCurSel(0);
    ShowTab(0);
    m_bUiInitialized = TRUE;
    Invalidate();
    ModernUIWindow::ApplyWhiteTitleBar(this->GetSafeHwnd());
    return TRUE;
}
// ============================================================================
// CalculateRequiredHeight - 탭 UI 기준 높이
// ============================================================================
int CShopSetupDlg::CalculateRequiredHeight()
{
    /* [UI-STEP] 현재 탭에서 필요한 전체 높이 계산(스크롤/자동 리사이즈 기준)
     * 1) 탭별로 배치되는 마지막 컨트롤의 하단 Y를 추적한다.
     * 2) 하단 마진을 더해 '필요 전체 높이'를 반환한다.
     * 3) 현재 클라이언트 높이보다 크면 스크롤/클리핑 처리 기준으로 사용한다.
     */

    // ── 카드 공통 파라미터 (ApplyLayout과 동일 값) ─────────────────
    const int FIELD_H = SX(44);
    const int cOutY = SX(12);   // 카드 외부 상단
    const int cGapY = SX(12);   // 카드 간 간격
    const int cPadY = SX(16);   // 카드 내부 상하
    const int cHdrH = SX(44);   // 카드 헤더 높이
    const int capH = SX(18);   // 라벨 높이
    const int capG = SX(7);    // 라벨→컨트롤 간격
    const int rG = SX(20);   // 행 간격
    auto oneRow = [&]() { return capH + capG + FIELD_H; };
    auto cardH = [&](int rows, int extraChecks = 0) -> int {
        // rows: 라벨+컨트롤 행 수, extraChecks: 추가 체크박스 행 수
        return cPadY + cHdrH + oneRow() * rows + rG * (rows - 1) + FIELD_H * extraChecks + cPadY;
        };
    int maxTabH = 0;
    // Tab 0: 결제 설정 (서버카드 2행 + 결제카드 3행)
    {
        int h = cOutY + cardH(2) + cGapY + cardH(3) + 8;
        maxTabH = max(maxTabH, h);
    }
    // Tab 1: 장치 정보 (리더기 2행 + 서명패드 2행 + 기타 체크1행)
    {
        // 리더기: 1행, 서명패드: 2행, 기타: 헤더+체크1행
        int card1 = cPadY + cHdrH + oneRow() * 1 + cPadY;
        int card2 = cPadY + cHdrH + oneRow() * 2 + rG + cPadY;
        int card3 = cPadY + cHdrH + oneRow() + rG + FIELD_H + cPadY;
        int h = cOutY + card1 + cGapY + card2 + cGapY + card3 + SX(10);
        maxTabH = max(maxTabH, h);
    }
    // Tab 2: 시스템 설정 (알림창 2행+체크1행 + 시스템 체크1행 + 단축키 1행)
    {
        int card1 = cPadY + cHdrH + oneRow() * 2 + rG + FIELD_H + cPadY;  // 알림창
        int card2 = cPadY + cHdrH + FIELD_H + cPadY;                     // 시스템
        int card3 = cPadY + cHdrH + oneRow() + cPadY;                    // 단축키
        int h = cOutY + card1 + cGapY + card2 + cGapY + card3;
        maxTabH = max(maxTabH, h);
    }
    // Tab 3: 가맹점 다운로드
    {
        int h = SX(224);
        maxTabH = max(maxTabH, h);
    }
    const int TITLE_AREA = SX(kTabBarTop);
    const int TAB_H = SX(kTabBarH);
    const int PAD_TOP = SX(kTabPadTop);
    const int PAD_BOTTOM = SX(18);
    const int BUTTON_AREA = SX(76);   // [TUNE] 하단 버튼 영역 높이 (버튼H 36 + 상하여백)
    const int CARD_PAD = SX(28);  // [NOTE] 실제로는 CARD_PAD/2만큼 활용
    return CARD_PAD + TITLE_AREA + TAB_H + PAD_TOP + maxTabH + PAD_BOTTOM + BUTTON_AREA;
}
// ============================================================================
// EnsureFonts
// ============================================================================
void CShopSetupDlg::EnsureFonts()
{
    /* [UI-STEP] UI 폰트 초기화(가독성/위계 유지)
     * 1) DPI 스케일을 반영해 제목/본문/설명 텍스트용 폰트 크기를 산출한다.
     * 2) CreateFontIndirect 또는 CreatePointFont 계열로 폰트를 생성한다(실패 시 기본 폰트로 폴백).
     * 3) 생성한 폰트를 컨트롤들(라벨/버튼/에딧/콤보)에 SetFont로 적용한다.
     * 4) OnDestroy()에서 DeleteObject로 폰트 리소스를 반드시 해제한다.
     *
     * [참고]
     * - 폰트 객체는 GDI 리소스라서 누수되면 장시간 사용 시 그리기 이상/크래시 원인이 된다.
     */
    if (m_fontTitle.GetSafeHandle()) return;
    LOGFONT lf = { 0 };
    ::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    ModernUIFont::ApplyUIFontFace(lf);
    // Use base 96-DPI pixel sizes and scale at runtime (per-monitor DPI)
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 20);
    lf.lfWeight = FW_BOLD;
    m_fontTitle.DeleteObject();
    m_fontTitle.CreateFontIndirect(&lf);
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 11);
    lf.lfWeight = FW_NORMAL;
    m_fontSubtitle.DeleteObject();
    m_fontSubtitle.CreateFontIndirect(&lf);
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 13);
    lf.lfWeight = FW_BOLD;
    m_fontSection.DeleteObject();
    m_fontSection.CreateFontIndirect(&lf);
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 14);
    lf.lfWeight = FW_NORMAL;
    m_fontLabel.DeleteObject();
    m_fontLabel.CreateFontIndirect(&lf);
    lf.lfWeight = FW_BOLD;
    m_fontGroupTitle.DeleteObject();
    m_fontGroupTitle.CreateFontIndirect(&lf);
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 13);
    lf.lfWeight = FW_BOLD;
    m_fontValidation.DeleteObject();
    m_fontValidation.CreateFontIndirect(&lf);
}
// ============================================================================
// InitializeControls
// ============================================================================
void CShopSetupDlg::InitializeControls()
{
    /* [UI-STEP] 컨트롤 생성/연결(탭/입력/토글/팝오버 아이콘)
     * 1) 탭 컨트롤 생성 및 탭 아이템(결제 설정/장치 정보/시스템 설정 등)을 등록한다.
     * 2) 각 섹션별 라벨, Edit/Combo/Toggle 컨트롤을 생성하고 ID/멤버 변수와 연결한다.
     * 3) 정보 아이콘(i) 버튼을 생성하고, 클릭 핸들러(OnBnClickedXXXInfo)와 연결해 팝오버를 띄울 준비를 한다.
     * 4) 커스텀 컨트롤(스킨 에딧/콤보)의 테마(배경/보더/포커스 색)를 적용한다.
     * 5) 초기 상태(기본 선택/비활성화/툴팁 텍스트 등)를 세팅한다.
     *
     * [참고]
     * - 컨트롤 생성은 OnInitDialog에서 1회만 수행(반복 생성 금지).
     * - 동적 생성 컨트롤은 자식 윈도우 핸들이 유효한지(IsWindow) 체크 후 접근.
     */
    auto RemoveEdges = [&](int id)
        {
            CWnd* w = GetDlgItem(id);
            if (!w) return;
            w->ModifyStyle(WS_BORDER, 0, SWP_FRAMECHANGED);
            w->ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
        };
    RemoveEdges(IDC_COMBO_ALARM_POS);
    RemoveEdges(IDC_COMBO_ALARM_SIZE);
    RemoveEdges(IDC_COMBO_CANCEL_KEY);
    RemoveEdges(IDC_COMBO_CASH_RECEIPT);
    RemoveEdges(IDC_COMBO_COMM_TYPE);
    RemoveEdges(IDC_COMBO_INTERLOCK);
    RemoveEdges(IDC_COMBO_MSR_KEY);
    RemoveEdges(IDC_COMBO_SIGN_PAD_SPEED);
    RemoveEdges(IDC_COMBO_SIGN_PAD_USE);
    RemoveEdges(IDC_COMBO_VAN_SERVER);
    RemoveEdges(IDC_EDIT_CARD_DETECT_PARAM);
    RemoveEdges(IDC_EDIT_CARD_TIMEOUT);
    RemoveEdges(IDC_EDIT_NO_SIGN_AMOUNT);
    RemoveEdges(IDC_EDIT_PORT);
    RemoveEdges(IDC_EDIT_SCANNER_PORT);
    RemoveEdges(IDC_EDIT_SIGN_PAD_PORT);
    RemoveEdges(IDC_EDIT_TAX_PERCENT);
    COLORREF bgColor = RGB(250, 251, 253);
    m_btnOk.SubclassDlgItem(IDOK, this);
    m_btnCancel.SubclassDlgItem(IDCANCEL, this);
    m_btnOk.ModifyStyle(0, BS_OWNERDRAW);
    m_btnCancel.ModifyStyle(0, BS_OWNERDRAW);
    m_btnOk.SetUnderlayColor(RGB(255, 255, 255));
    m_btnCancel.SetUnderlayColor(RGB(255, 255, 255));
    m_btnOk.SetButtonStyle(ButtonStyle::Primary);
    m_btnCancel.SetButtonStyle(ButtonStyle::Default);
    m_comboVanServer.SetUnderlayColor(bgColor);
    m_comboCashReceipt.SetUnderlayColor(bgColor);
    m_comboInterlock.SetUnderlayColor(bgColor);
    m_comboCommType.SetUnderlayColor(bgColor);
    m_comboSignPadUse.SetUnderlayColor(bgColor);
    m_comboSignPadSpeed.SetUnderlayColor(bgColor);
    m_comboAlarmPos.SetUnderlayColor(bgColor);
    m_comboAlarmSize.SetUnderlayColor(bgColor);
    m_comboCancelKey.SetUnderlayColor(bgColor);
    m_comboMSRKey.SetUnderlayColor(bgColor);
    m_editPort.SetUnderlayColor(bgColor);
    m_editNoSignAmount.SetUnderlayColor(bgColor);
    m_editTaxPercent.SetUnderlayColor(bgColor);
    m_editCardTimeout.SetUnderlayColor(bgColor);
    m_editCardTimeout.SetUnitText(_T("초"), 30);
    m_editNoSignAmount.SetUnitText(_T("원"), 30);
    m_editTaxPercent.SetUnitText(_T("%"), 25);
    m_editCardDetectParam.SetUnderlayColor(bgColor);
    m_editSignPadPort.SetUnderlayColor(bgColor);
    m_editScannerPort.SetUnderlayColor(bgColor);
    m_editPort.SetNumericOnly(TRUE);
    m_editNoSignAmount.SetNumericOnly(TRUE);
    m_editTaxPercent.SetNumericOnly(TRUE);
    m_editCardTimeout.SetNumericOnly(TRUE);
    m_editSignPadPort.SetNumericOnly(TRUE);
    m_editScannerPort.SetNumericOnly(TRUE);
    m_staticShopContainer.SubclassDlgItem(IDC_STATIC_RECT, this);
    m_staticShopContainer.ModifyStyle(0, WS_CLIPCHILDREN); // 컨테이너가 자식을 지우지 않게 함
    // 체크박스
    auto SetupTgl = [&](CModernToggleSwitch& sw, int id, LPCTSTR txt, BOOL bOn)
        {
            sw.SubclassDlgItem(id, this);
            sw.SetFont(&m_fontLabel);
            LOGFONT lfTmp_ = {}; m_fontLabel.GetLogFont(&lfTmp_); int labelPx_ = abs(lfTmp_.lfHeight);
            // 기존 체크박스 스타일 제거 + owner-draw 적용
            sw.ModifyStyle(BS_AUTOCHECKBOX | BS_CHECKBOX | BS_3STATE | BS_AUTO3STATE | BS_AUTORADIOBUTTON | BS_RADIOBUTTON, BS_OWNERDRAW);
            sw.ModifyStyle(WS_BORDER, 0);
            sw.ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0);
            sw.SetWindowPos(NULL, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            sw.SetToggled(bOn);
            sw.SetUnderlayColor(bgColor);
            sw.SetWindowText(txt);
            sw.RedrawWindow(NULL, NULL,
                RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW);
            sw.SetTextSizePx(labelPx_);  // fontLabel과 동일 (13px)
            sw.SetNoWrapEllipsis(TRUE);
        };
    SetupTgl(m_chkCardDetect, IDC_CHECK_CARD_DETECT, _T("카드 감지 우선 거래 사용"), FALSE);
    SetupTgl(m_chkMultiVoice, IDC_CHECK_MULTI_VOICE, _T("멀티패드 음성 출력"), FALSE);
    SetupTgl(m_chkScannerUse, IDC_CHECK_SCANNER_USE, _T("스캐너 사용"), FALSE);
    SetupTgl(m_chkAlarmGraph, IDC_CHECK_ALARM_GRAPH, _T("알림창 그림"), TRUE);
    SetupTgl(m_chkAlarmDual, IDC_CHECK_ALARM_DUAL, _T("알림창 듀얼"), FALSE);
    SetupTgl(m_chkAutoReboot, IDC_CHECK_AUTO_REBOOT, _T("자동 리부팅"), TRUE);
    if (!::IsWindow(::GetDlgItem(m_hWnd, IDC_STATIC_CARD_DETECT_POSINFO)))
    {
        HWND hStatic = ::CreateWindowEx(
            0,
            _T("STATIC"),
            _T("우선 거래 프로그램"),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hWnd,
            (HMENU)IDC_STATIC_CARD_DETECT_POSINFO,
            AfxGetInstanceHandle(),
            NULL);
        if (hStatic)
        {
            ::SendMessage(hStatic, WM_SETFONT, (WPARAM)(HFONT)m_fontLabel.GetSafeHandle(), TRUE);
        }
    }
    if (!::IsWindow(::GetDlgItem(m_hWnd, IDC_STATIC_SCANNER_PORT_LABEL)))
    {
        HWND hStatic = ::CreateWindowEx(
            0,
            _T("STATIC"),
            _T("스캐너 포트번호"),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hWnd,
            (HMENU)IDC_STATIC_SCANNER_PORT_LABEL,
            AfxGetInstanceHandle(),
            NULL);
        if (hStatic)
        {
            ::SendMessage(hStatic, WM_SETFONT, (WPARAM)(HFONT)m_fontLabel.GetSafeHandle(), TRUE);
        }
    }
    // 콤보박스 초기화
    // 콤보박스 초기화 (Word 스펙 기준 목록)
    FillCombo(m_comboVanServer, kVanServers, (int)(sizeof(kVanServers) / sizeof(kVanServers[0])));
    FillCombo(m_comboCashReceipt, kCashReceipt, (int)(sizeof(kCashReceipt) / sizeof(kCashReceipt[0])));
    FillCombo(m_comboInterlock, kInterlock, (int)(sizeof(kInterlock) / sizeof(kInterlock[0])));
    FillCombo(m_comboCommType, kCommType, (int)(sizeof(kCommType) / sizeof(kCommType[0])));
    FillCombo(m_comboSignPadUse, kSignPadUse, (int)(sizeof(kSignPadUse) / sizeof(kSignPadUse[0])));
    FillCombo(m_comboSignPadSpeed, kSignPadSpeed, (int)(sizeof(kSignPadSpeed) / sizeof(kSignPadSpeed[0])));
    FillCombo(m_comboAlarmPos, kAlarmPos, (int)(sizeof(kAlarmPos) / sizeof(kAlarmPos[0])));
    FillCombo(m_comboAlarmSize, kAlarmSize, (int)(sizeof(kAlarmSize) / sizeof(kAlarmSize[0])));
    FillCombo(m_comboCancelKey, kHotkeys, (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])));
    FillCombo(m_comboMSRKey, kHotkeys, (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])));
    // 라벨 폰트
    const int lblIds[] = {
        IDC_STATIC_VAN_SERVER, IDC_STATIC_PORT,
        IDC_STATIC_CARD_TIMEOUT, IDC_STATIC_NO_SIGN_AMOUNT, IDC_STATIC_TAX_PERCENT,
        IDC_STATIC_CASH_RECEIPT, IDC_STATIC_INTERLOCK,
        IDC_STATIC_COMM_TYPE, IDC_STATIC_SIGN_PAD_USE, IDC_STATIC_SIGN_PAD_PORT,
        IDC_STATIC_SIGN_PAD_SPEED, IDC_STATIC_ALARM_POS, IDC_STATIC_ALARM_SIZE,
        IDC_STATIC_CANCEL_KEY, IDC_STATIC_MSR_KEY, IDC_STATIC_CARD_DETECT_POSINFO, IDC_STATIC_SCANNER_PORT_LABEL
    };
    for (int id : lblIds)
    {
        CWnd* p = GetDlgItem(id);
        if (p) p->SetFont(&m_fontLabel);
    }
    // Apply Pretendard font to edit controls (input text) and combo boxes (dropdown list)
    CWnd* inputControls[] = {
        &m_editPort, &m_editNoSignAmount, &m_editTaxPercent, &m_editCardTimeout,
        &m_editCardDetectParam, &m_editSignPadPort, &m_editScannerPort,
        &m_comboVanServer, &m_comboCashReceipt, &m_comboInterlock, &m_comboCommType,
        &m_comboSignPadUse, &m_comboSignPadSpeed, &m_comboAlarmPos, &m_comboAlarmSize,
        &m_comboCancelKey, &m_comboMSRKey
    };
    for (CWnd* w : inputControls)
        w->SetFont(&m_fontLabel);
    for (CSkinnedComboBox* cb : { &m_comboVanServer, &m_comboCashReceipt, &m_comboInterlock, &m_comboCommType, &m_comboSignPadUse, &m_comboSignPadSpeed, &m_comboAlarmPos, &m_comboAlarmSize, &m_comboCancelKey, &m_comboMSRKey })
        cb->SetTextPx(14);
}
// --- SX: DPI-aware scaling shorthand ---
int CShopSetupDlg::SX(int px) const
{
    return ModernUIDpi::Scale(m_hWnd, px);
}
// --- MoveCtrl: move a dialog control; ComboBox gets standard drop height ---
void CShopSetupDlg::MoveCtrl(int nID, int x, int y, int w, int h, BOOL bShow)
{
    /* [UI-STEP] 컨트롤 이동 공용 헬퍼(좌표/크기 적용)
     * 1) 대상 컨트롤 HWND가 유효한지 확인한다.
     * 2) SetWindowPos/MoveWindow로 좌표/크기를 적용한다.
     * 3) 필요 시 SWP_NOZORDER/SWP_NOACTIVATE 등 플래그로 포커스/순서를 보호한다.
     *
     * [참고]
     * - MoveWindow는 내부적으로 WM_SIZE/WM_WINDOWPOSCHANGED를 유발할 수 있어, 레이아웃 중 재진입을 조심.
     */
    CWnd* p = GetDlgItem(nID);
    if (!p || !p->GetSafeHwnd()) return;
    TCHAR cls[64] = { 0 };
    ::GetClassName(p->GetSafeHwnd(), cls, 63);
    if (_tcsicmp(cls, _T("ComboBox")) == 0)
    {
        p->MoveWindow(x, y, w, SX(220));
        ::SendMessage(p->GetSafeHwnd(), CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)(h - 2));
        ::SendMessage(p->GetSafeHwnd(), CB_SETITEMHEIGHT, (WPARAM)0, (LPARAM)(h - 2));
    }
    else
    {
        p->MoveWindow(x, y, w, h);
    }
    p->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
}
// --- Tab 0: card reader settings ---
// --------------------------------------------------------------
// 레이아웃 배치
//  - 96dpi 기준 상수를 ModernUIDpi::Scale()로 변환하여 배치
//  - 리사이즈/탭 전환 시에도 호출 가능하도록, '배치만' 담당하는 것이 이상적
// --------------------------------------------------------------
void CShopSetupDlg::ApplyLayoutTab0()
{
    /* [UI-STEP] 탭0(결제 설정) 영역 배치
     * 1) 탭0에서 표시할 그룹(예: 금융결제원 서버/포트/통신방식/현금영수증 등)의 시작 Y 좌표를 결정한다.
     * 2) 각 행(라벨 + 입력 컨트롤 + 정보아이콘)을 동일한 기준선/높이로 배치한다.
     * 3) 콤보/에딧의 폭은 '라벨 폭 + 입력 폭' 규칙에 따라 정렬되도록 맞춘다.
     * 4) 그룹 간 간격을 적용하고 다음 그룹으로 y를 진행한다.
     */

    auto Move = [&](int id, int x, int y, int w, int h) { MoveCtrl(id, x, y, w, h); };
    const int CTRL_H = SX(40);
    const int FIELD_H = CTRL_H;
    CClientDC measureDC(this);
    auto PlaceInfoBtn = [&](CInfoIconButton& btn, int labelId, int lx, int ly, int lcapH) {
        if (!btn.GetSafeHwnd()) return;
        const int BtnSz = SX(18);
        const int BtnGap = SX(4);
        int bx = lx + BtnGap;
        int by = ly + (lcapH - BtnSz) / 2;
        CWnd* pLbl = GetDlgItem(labelId);
        if (pLbl && pLbl->GetSafeHwnd()) {
            CFont* pFont = pLbl->GetFont();
            CFont* pOld = pFont ? measureDC.SelectObject(pFont) : NULL;
            CString strLbl;
            pLbl->GetWindowText(strLbl);
            CSize sz = measureDC.GetTextExtent(strLbl);
            if (pOld) measureDC.SelectObject(pOld);
            bx = lx + sz.cx + BtnGap;
        }
        btn.SetWindowPos(NULL, bx, by, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
        };
    int y = m_rcTabContent.top + SX(kTabPadTop);
    const int cardOuterPadX = 16;
    const int cardOuterPadY = 12;
    const int cardGapY = 16;
    const int cardPadX = 22;
    const int cardPadY = 16;   // 상하
    const int headerH = 44;   // 불릿헤더
    const int capH = 18;   // [TUNE] 캡션(라벨) 높이
    const int capGap = 7;    // 캡션간격
    const int rowGap = 20;   // 행간격
    const int colGap = 20;
    int cardLeft = m_rcTabContent.left + cardOuterPadX;
    int cardRight = m_rcTabContent.right - cardOuterPadX;
    int cardW = cardRight - cardLeft;
    int curY = y + cardOuterPadY;
    // -------------------------
    // 카드 1) 서버 설정
    // -------------------------
    int innerX = cardLeft + cardPadX;
    int innerW = cardW - cardPadX * 2;
    int y1 = curY + cardPadY + headerH;
    // 금융결제원 서버 / 포트번호 (한 줄 2열)
    int colW = (innerW - colGap) / 2;
    // 좌: 금융결제원 서버
    Move(IDC_STATIC_VAN_SERVER, innerX, y1, colW, capH);
    Move(IDC_COMBO_VAN_SERVER, innerX, y1 + capH + capGap, colW, FIELD_H);
    PlaceInfoBtn(m_btnVanInfo, IDC_STATIC_VAN_SERVER, innerX, y1, capH);
    // 우: 포트번호
    Move(IDC_STATIC_PORT, innerX + colW + colGap, y1, colW, capH);
    PlaceInfoBtn(m_btnPortInfo, IDC_STATIC_PORT, innerX + colW + colGap, y1, capH);
    Move(IDC_EDIT_PORT, innerX + colW + colGap, y1 + capH + capGap, colW, FIELD_H);
    PositionValidationText(IDC_STATIC_ERR_PORT, innerX + colW + colGap + colW - SX(88), y1, SX(88), capH);
    y1 += capH + capGap + FIELD_H;
    int serverCardH = (y1 + cardPadY) - curY;
    m_rcCardServer = CRect(cardLeft, curY, cardRight, curY + serverCardH);
    curY = m_rcCardServer.bottom + cardGapY;
    // -------------------------
    // 카드 2) 결제 방식
    // -------------------------
    innerX = cardLeft + cardPadX;
    innerW = cardW - cardPadX * 2;
    int y2 = curY + cardPadY + headerH;
    // 통신방식 / 현금영수증 거래 (2열)
    colW = (innerW - colGap) / 2;
    Move(IDC_STATIC_COMM_TYPE, innerX, y2, colW, capH);
    PlaceInfoBtn(m_btnCommTypeInfo, IDC_STATIC_COMM_TYPE, innerX, y2, capH);
    Move(IDC_COMBO_COMM_TYPE, innerX, y2 + capH + capGap, colW, FIELD_H);
    int rx = innerX + colW + colGap;
    Move(IDC_STATIC_CASH_RECEIPT, rx, y2, colW, capH);
    PlaceInfoBtn(m_btnCashReceiptInfo, IDC_STATIC_CASH_RECEIPT, rx, y2, capH);
    Move(IDC_COMBO_CASH_RECEIPT, rx, y2 + capH + capGap, colW, FIELD_H);
    y2 += capH + capGap + FIELD_H + rowGap;
    // 무서명 / 카드 감지 우선 거래 사용 (2열)
    Move(IDC_STATIC_NO_SIGN_AMOUNT, innerX, y2, colW, capH);
    Move(IDC_EDIT_NO_SIGN_AMOUNT, innerX, y2 + capH + capGap, colW, FIELD_H);
    PositionValidationText(IDC_STATIC_ERR_NO_SIGN, innerX + colW - SX(88), y2, SX(88), capH);
    {
        const int BtnSz = SX(18);
        const int BtnGap = SX(6);
        const int toggleY = y2 + capH + capGap;
        const int rightX = innerX + colW + colGap;
        // 카드 감지 우선 거래 사용: 세금 자동 역산 자리(2행 오른쪽)로 이동
        // 토글 오른쪽에 팝오버 아이콘 영역을 확보해서 겹치지 않게 배치
        int toggleW = colW - (BtnSz + BtnGap);
        if (toggleW < SX(110))
            toggleW = max(1, colW - (BtnSz + BtnGap));
        Move(IDC_CHECK_CARD_DETECT, rightX, toggleY, toggleW, FIELD_H);
        if (m_btnCardDetectInfo.GetSafeHwnd())
        {
            int ibX = rightX + toggleW + BtnGap;
            int ibY = toggleY + (FIELD_H - BtnSz) / 2;
            m_btnCardDetectInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    y2 += capH + capGap + FIELD_H + rowGap;
    // 세금 자동 역산 / 우선 거래 프로그램 (2열)
    Move(IDC_STATIC_TAX_PERCENT, innerX, y2, colW, capH);
    PlaceInfoBtn(m_btnTaxPercentInfo, IDC_STATIC_TAX_PERCENT, innerX, y2, capH);
    Move(IDC_EDIT_TAX_PERCENT, innerX, y2 + capH + capGap, colW, FIELD_H);
    PositionValidationText(IDC_STATIC_ERR_TAX, innerX + colW - SX(88), y2, SX(88), capH);
    Move(IDC_STATIC_CARD_DETECT_POSINFO, innerX + colW + colGap, y2, colW, capH);
    Move(IDC_EDIT_CARD_DETECT_PARAM, innerX + colW + colGap, y2 + capH + capGap, colW, FIELD_H);
    PositionValidationText(IDC_STATIC_ERR_CARD_DETECT_PROGRAM, innerX + colW + colGap + colW - SX(88), y2, SX(88), capH);
    y2 += capH + capGap + FIELD_H;
    int payCardH = (y2 + cardPadY) - curY;
    m_rcCardPayMethod = CRect(cardLeft, curY, cardRight, curY + payCardH);
    // Sync edit heights to match combo closed height
    {
        const int editIdsTab0[] = { IDC_EDIT_PORT, IDC_EDIT_NO_SIGN_AMOUNT, IDC_EDIT_TAX_PERCENT, IDC_EDIT_CARD_DETECT_PARAM };
        NormalizeInputHeightsToCombo(this, IDC_COMBO_VAN_SERVER, editIdsTab0, 4);
    }
}
// --- Tab 1+2: devices and system settings ---
void CShopSetupDlg::ApplyLayoutTab1()
{
    /* [UI-STEP] 탭1(장치 정보) 영역 배치
     * 1) 탭1에서 표시할 그룹(카드입력 Timeout, 장치 연동 방식, 서명패드 설정 등)을 순서대로 배치한다.
     * 2) 토글이 있는 행은 '라벨 + 토글 + 정보아이콘' 정렬 규칙을 적용한다.
     * 3) 서명패드 관련 입력은 종속 관계(사용 ON일 때만 활성화 등)가 있으면 EnableWindow로 제어한다.
     */

    auto Move = [&](int id, int x, int y, int w, int h) { MoveCtrl(id, x, y, w, h); };
    const int CTRL_H = SX(40);
    const int FIELD_H = CTRL_H;
    CClientDC measureDC(this);
    auto PlaceInfoBtn = [&](CInfoIconButton& btn, int labelId, int lx, int ly, int lcapH) {
        if (!btn.GetSafeHwnd()) return;
        const int BtnSz = SX(18);
        const int BtnGap = SX(4);
        int bx = lx + BtnGap;
        int by = ly + (lcapH - BtnSz) / 2;
        CWnd* pLbl = GetDlgItem(labelId);
        if (pLbl && pLbl->GetSafeHwnd()) {
            CFont* pFont = pLbl->GetFont();
            CFont* pOld = pFont ? measureDC.SelectObject(pFont) : NULL;
            CString strLbl;
            pLbl->GetWindowText(strLbl);
            CSize sz = measureDC.GetTextExtent(strLbl);
            if (pOld) measureDC.SelectObject(pOld);
            bx = lx + sz.cx + BtnGap;
        }
        btn.SetWindowPos(NULL, bx, by, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
        };
    int y = m_rcTabContent.top + SX(kTabPadTop);
    const int cOutX = 16;   // [TUNE] 카드 외부 좌우 여백
    const int cOutY = 12;   // [TUNE] 카드 외부 상단 여백
    const int cGapY = 12;   // [TUNE] 카드 간 세로 간격
    const int cPadX = 22;   // [TUNE] 카드 내부 좌우 여백
    const int cPadY = 16;   // [TUNE] 카드 내부 상하 여백
    const int cHdrH = 44;   // [TUNE] 카드 헤더 높이
    const int capH = 18;   // [TUNE] 라벨 텍스트 높이
    const int capG = 7;    // [TUNE] 라벨→컨트롤 간격
    const int rG = 20;   // [TUNE] 카드 내 행 간격
    const int cG = 18;   // [TUNE] 열 간격
    const int chkW = 140;  // [TUNE] 체크박스 1개 폭 (3개/행 기준)
    int cLeft = m_rcTabContent.left + cOutX;
    int cRight = m_rcTabContent.right - cOutX;
    int cW = cRight - cLeft;
    int curY = y + cOutY;
    int inX = cLeft + cPadX;
    int inW = cW - cPadX * 2;
    int col2W = (inW - cG) / 2;  // 2열 분할 폭
    // =================================================================
    // Tab 1: 장치 정보
    // =================================================================
        // ── 카드 1: 리더기 ──────────────────────────────
    {
        int fy = curY + cPadY + cHdrH;
        // 행1: 카드 응답 타임아웃 / 연동 방식 (2열)
        Move(IDC_STATIC_CARD_TIMEOUT, inX, fy, col2W, capH);
        PlaceInfoBtn(m_btnCardTimeoutInfo, IDC_STATIC_CARD_TIMEOUT, inX, fy, capH);
        Move(IDC_EDIT_CARD_TIMEOUT, inX, fy + capH + capG, col2W, FIELD_H);
        PositionValidationText(IDC_STATIC_ERR_TIMEOUT, inX + col2W - SX(88), fy, SX(88), capH);
        Move(IDC_STATIC_INTERLOCK, inX + col2W + cG, fy, col2W, capH);
        PlaceInfoBtn(m_btnInterlockInfo, IDC_STATIC_INTERLOCK, inX + col2W + cG, fy, capH);
        Move(IDC_COMBO_INTERLOCK, inX + col2W + cG, fy + capH + capG, col2W, FIELD_H);
        fy += capH + capG + FIELD_H + rG;
        int cardH = (fy + cPadY) - curY;
        m_rcGrpReader = CRect(cLeft, curY, cRight, curY + cardH);
        curY = m_rcGrpReader.bottom + cGapY;
    }
    // ── 카드 2: 서명패드 ──────────────────────────────
    {
        int fy = curY + cPadY + cHdrH;
        // 행1: 서명패드 사용 / 포트번호 (2열)
        Move(IDC_STATIC_SIGN_PAD_USE, inX, fy, col2W, capH);
        PlaceInfoBtn(m_btnSignPadUseInfo, IDC_STATIC_SIGN_PAD_USE, inX, fy, capH);
        Move(IDC_COMBO_SIGN_PAD_USE, inX, fy + capH + capG, col2W, FIELD_H);
        Move(IDC_STATIC_SIGN_PAD_PORT, inX + col2W + cG, fy, col2W, capH);
        PlaceInfoBtn(m_btnSignPadPortInfo, IDC_STATIC_SIGN_PAD_PORT, inX + col2W + cG, fy, capH);
        Move(IDC_EDIT_SIGN_PAD_PORT, inX + col2W + cG, fy + capH + capG, col2W, FIELD_H);
        PositionValidationText(IDC_STATIC_ERR_SIGNPAD_PORT, inX + col2W + cG + col2W - SX(88), fy, SX(88), capH);
        fy += capH + capG + FIELD_H + rG;
        // 행2: 통신속도 (1열, 반폭)
        Move(IDC_STATIC_SIGN_PAD_SPEED, inX, fy, col2W, capH);
        PlaceInfoBtn(m_btnSignPadSpeedInfo, IDC_STATIC_SIGN_PAD_SPEED, inX, fy, capH);
        Move(IDC_COMBO_SIGN_PAD_SPEED, inX, fy + capH + capG, col2W, FIELD_H);
        fy += capH + capG + FIELD_H;
        int cardH = (fy + cPadY) - curY;
        m_rcGrpSign = CRect(cLeft, curY, cRight, curY + cardH);
        curY = m_rcGrpSign.bottom + cGapY;
    }
    // ── 카드 3: 기타 ────────────────────────────────
    {
        const int etcTopTight = SX(4);   // [TUNE] 기타 카드 첫 줄을 조금 위로 당긴다.
        const int etcRowGap = SX(10);  // [TUNE] 토글 행과 포트번호 행 간격을 줄인다.
        const int etcBottomPad = SX(10);  // [TUNE] 카드 하단 여백을 줄여 버튼과 겹치지 않게 한다.
        int fy = curY + cPadY + cHdrH - etcTopTight;
        const int BtnSz = SX(18);
        const int BtnGap = SX(6);
        const int leftX = inX;
        const int rightX = inX + col2W + cG;
        const int toggleY = fy;
        // 행1 좌측: 스캐너 사용 토글 + 팝오버
        {
            int toggleW = col2W - (BtnSz + BtnGap);
            if (toggleW < SX(110))
                toggleW = max(1, col2W - (BtnSz + BtnGap));
            Move(IDC_CHECK_SCANNER_USE, leftX, toggleY, toggleW, FIELD_H);
            if (m_btnScannerUseInfo.GetSafeHwnd())
            {
                int ibX = leftX + toggleW + BtnGap;
                int ibY = toggleY + (FIELD_H - BtnSz) / 2;
                m_btnScannerUseInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        // 행1 우측: 멀티패드 음성 출력 토글 + 팝오버
        {
            int mvW = col2W - (BtnSz + BtnGap);
            if (mvW < SX(110))
                mvW = max(1, col2W - (BtnSz + BtnGap));
            Move(IDC_CHECK_MULTI_VOICE, rightX, toggleY, mvW, FIELD_H);
            if (m_btnMultiVoiceInfo.GetSafeHwnd())
            {
                int ibX = rightX + mvW + BtnGap;
                int ibY = toggleY + (FIELD_H - BtnSz) / 2;
                m_btnMultiVoiceInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        fy = toggleY + FIELD_H + etcRowGap;
        // 행2 좌측: 스캐너 포트번호 + EditText
        Move(IDC_STATIC_SCANNER_PORT_LABEL, leftX, fy, col2W, capH);
        Move(IDC_EDIT_SCANNER_PORT, leftX, fy + capH + capG, col2W, FIELD_H);
        PositionValidationText(IDC_STATIC_ERR_SCANNER_PORT, leftX + col2W - SX(88), fy, SX(88), capH);
        int cardH = (fy + capH + capG + FIELD_H + etcBottomPad) - curY;
        m_rcGrpEtc = CRect(cLeft, curY, cRight, curY + cardH);
        // curY = m_rcGrpEtc.bottom + cGapY;  // Tab1 끝
    }
    // =================================================================
        // Tab 2: 시스템 설정
        // =================================================================
    curY = y + cOutY;
    // ── 카드 1: 알림창 설정 ─────────────────────────
    {
        int fy = curY + cPadY + cHdrH;
        // 행1: 알림창 크기 / 알림창 위치 (2열)
        Move(IDC_STATIC_ALARM_SIZE, inX, fy, col2W, capH);
        PlaceInfoBtn(m_btnAlarmSizeInfo, IDC_STATIC_ALARM_SIZE, inX, fy, capH);
        Move(IDC_COMBO_ALARM_SIZE, inX, fy + capH + capG, col2W, FIELD_H);
        Move(IDC_STATIC_ALARM_POS, inX + col2W + cG, fy, col2W, capH);
        Move(IDC_COMBO_ALARM_POS, inX + col2W + cG, fy + capH + capG, col2W, FIELD_H);
        fy += capH + capG + FIELD_H + rG;
        // 행2: 체크박스 3개 (그래프/원상복구/듀얼모니터)
        int chk2W = (inW - cG) / 2;
        // 알림창 옵션: 토글 오른쪽에 팝오버 아이콘 영역 확보(겹치지 않게)
        {
            const int BtnSz = SX(18);
            const int BtnGap = SX(4);
            int iconNeed = BtnSz + BtnGap;
            int wL = chk2W;
            int wR = chk2W;
            if (wL > SX(80) + iconNeed) wL -= iconNeed;
            if (wR > SX(80) + iconNeed) wR -= iconNeed;
            int xL = inX;
            int xR = inX + chk2W + cG;
            Move(IDC_CHECK_ALARM_GRAPH, xL, fy, wL, FIELD_H);
            if (m_btnAlarmGraphInfo.GetSafeHwnd())
            {
                int ibX = xL + wL + BtnGap;
                int ibY = fy + (FIELD_H - BtnSz) / 2;
                m_btnAlarmGraphInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            Move(IDC_CHECK_ALARM_DUAL, xR, fy, wR, FIELD_H);
            if (m_btnAlarmDualInfo.GetSafeHwnd())
            {
                int ibX = xR + wR + BtnGap;
                int ibY = fy + (FIELD_H - BtnSz) / 2;
                m_btnAlarmDualInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        fy += FIELD_H;
        int cardH = (fy + cPadY) - curY;
        m_rcGrpAlarm = CRect(cLeft, curY, cRight, curY + cardH);
        curY = m_rcGrpAlarm.bottom + cGapY;
    }
    // ── 카드 2: 시스템 ──────────────────────────────
    {
        int fy = curY + cPadY + cHdrH;
        {
            const int BtnSz = SX(18);
            const int BtnGap = SX(4);
            int iconNeed = BtnSz + BtnGap;
            int wL = (inW - cG) / 2;
            if (wL > SX(80) + iconNeed) wL -= iconNeed;
            int xL = inX;
            Move(IDC_CHECK_AUTO_REBOOT, xL, fy, wL, FIELD_H);
            if (m_btnAutoRebootInfo.GetSafeHwnd())
            {
                int ibX = xL + wL + BtnGap;
                int ibY = fy + (FIELD_H - BtnSz) / 2;
                m_btnAutoRebootInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        fy += FIELD_H;
        int cardH = (fy + cPadY) - curY;
        m_rcGrpSystem = CRect(cLeft, curY, cRight, curY + cardH);
        curY = m_rcGrpSystem.bottom + cGapY;
    }
    // ── 카드 3: 단축키 ──────────────────────────────
    {
        int fy = curY + cPadY + cHdrH;
        // 행1: 취소키 / MSR키 (2열)
        Move(IDC_STATIC_CANCEL_KEY, inX, fy, col2W, capH);
        Move(IDC_COMBO_CANCEL_KEY, inX, fy + capH + capG, col2W, FIELD_H);
        Move(IDC_STATIC_MSR_KEY, inX + col2W + cG, fy, col2W, capH);
        Move(IDC_COMBO_MSR_KEY, inX + col2W + cG, fy + capH + capG, col2W, FIELD_H);
        fy += capH + capG + FIELD_H;
        int cardH = (fy + cPadY) - curY;
        m_rcGrpHotkey = CRect(cLeft, curY, cRight, curY + cardH);
    // Sync edit heights to match combo closed height
    {
        const int editIdsTab1[] = { IDC_EDIT_CARD_TIMEOUT, IDC_EDIT_SIGN_PAD_PORT, IDC_EDIT_SCANNER_PORT };
        NormalizeInputHeightsToCombo(this, IDC_COMBO_INTERLOCK, editIdsTab1, 3);
    }
        // curY = m_rcGrpHotkey.bottom + cGapY;  // Tab2 끝
    }
}
// --- Tab 3: merchant download ---
void CShopSetupDlg::ApplyLayoutTab3()
{
    /* [UI-STEP] 탭3(시스템 설정) 영역 배치
     * 1) 알림창 크기 등 시스템 관련 옵션 UI를 배치한다.
     * 2) 콤보/에딧 폭을 전체 레이아웃 폭에 맞추고, 우측 여백을 통일한다.
     */

    CRect rc;
    GetClientRect(&rc);
    int y = m_rcTabContent.top + SX(kTabPadTop);
    // Tab1/2와 동일한 카드 외부/내부 파라미터를 사용해 정렬을 맞춘다.
    const int cOutX = 16;   // 카드 외부 좌우 여백
    const int cOutY = 12;   // 카드 외부 상단 여백
    const int cPadX = 22;   // 카드 내부 좌우 여백
    const int cPadY = 16;   // 카드 내부 상하 여백
    const int cHdrH = 44;   // 카드 헤더 높이(세로선/타이틀 영역)
    const int hostGapBottom = 14;
    const int cardBottomPad = 18;
    const int BUTTON_H = 36;
    const int BUTTON_BOTTOM = 22;
    // 하단 버튼 윗쪽까지를 카드 영역으로 사용
    int btnY = rc.bottom - (cardBottomPad + BUTTON_BOTTOM + BUTTON_H);
    int cardLeft = m_rcTabContent.left + SX(cOutX);
    int cardRight = m_rcTabContent.right - SX(cOutX);
    int cardTop = y + SX(cOutY);
    int cardBot = btnY - SX(cardBottomPad);
    if (cardBot < cardTop + SX(240))
        cardBot = cardTop + SX(240);
    m_rcCardShopDown = CRect(cardLeft, cardTop, cardRight, cardBot);
    // Child(ShopDownDlg)는 카드 내부 컨텐츠 영역(헤더 아래)만 차지하도록 한다.
    CRect rcHost(
        cardLeft + SX(cPadX),
        cardTop + SX(cHdrH + cPadY),
        cardRight - SX(cPadX),
        cardBot - SX(cPadY + hostGapBottom));
    if (rcHost.Height() < SX(200))
        rcHost.bottom = rcHost.top + SX(200);
    if (m_staticShopContainer.GetSafeHwnd())
    {
        m_staticShopContainer.MoveWindow(rcHost);
        m_staticShopContainer.ShowWindow(SW_HIDE); // ShowTab에서 처리
    }
}
// ============================================================================
// ApplyLayout - 탭 컨트롤 배치 + 각 탭 컨텐츠 배치
// ============================================================================
void CShopSetupDlg::ApplyLayout()
{
    /* [UI-STEP] 전체 레이아웃 엔트리(탭별 레이아웃 분기 + 스크롤/높이 계산)
     * 1) 현재 클라이언트 영역 크기 및 DPI 스케일(96dpi 기준)을 계산한다.
     * 2) 상단/좌측/우측/하단 마진과 그룹 간 간격 같은 레이아웃 상수를 스케일 적용한다.
     * 3) 현재 선택된 탭 인덱스를 확인하고, 탭별 배치 함수(ApplyLayoutTab0/1/3 등)로 분기한다.
     * 4) 필요 높이(CalculateRequiredHeight)와 현재 높이를 비교해 스크롤/클리핑 정책을 정한다(필요 시).
     * 5) MoveCtrl() 헬퍼로 각 컨트롤 위치/크기를 설정하고, 마지막에 Invalidate()로 재그림한다.
     *
     * [참고]
     * - 레이아웃 함수는 '값 계산'과 'MoveWindow/SetWindowPos'를 한 눈에 구분되게 두는 편이 유지보수에 좋다.
     * - 컨트롤이 많으니 배치 중 불필요한 Invalidate 반복을 피하고, 마지막에 1회 갱신하는 방식이 성능에 유리하다.
     */
    CRect rc;
    GetClientRect(&rc);

    // 헬퍼: 라벨 텍스트 오른쪽에 인포 아이콘 버튼 배치
    CClientDC measureDC(this);
    auto PlaceInfoBtn = [&](CInfoIconButton& btn, int labelId, int lx, int ly, int lcapH) {
        if (!btn.GetSafeHwnd()) return;
        const int BtnSz = SX(18);
        const int BtnGap = SX(4);
        int bx = lx + BtnGap;
        int by = ly + (lcapH - BtnSz) / 2;
        CWnd* pLbl = GetDlgItem(labelId);
        if (pLbl && pLbl->GetSafeHwnd()) {
            CFont* pFont = pLbl->GetFont();
            CFont* pOld = pFont ? measureDC.SelectObject(pFont) : NULL;
            CString strLbl;
            pLbl->GetWindowText(strLbl);
            CSize sz = measureDC.GetTextExtent(strLbl);
            if (pOld) measureDC.SelectObject(pOld);
            bx = lx + sz.cx + BtnGap;
        }
        btn.SetWindowPos(NULL, bx, by, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
        };
    const int MARGIN = SX(kTabPadLeft);
    const int LABEL_W = SX(92);
    const int FIELD_W = SX(120);
    const int LF_GAP = SX(8);
    const int FIELD_W_IN = FIELD_W - LF_GAP;
    const int CTRL_H = SX(40);   // [TUNE] 컨트롤 시각적 높이 (Edit/Combo 동일)
    const int FIELD_H = CTRL_H;  // 하위 호환용 alias
    const int COMBO_DROP_H = SX(220); // [TUNE] combo drop list height
    const int ROW_GAP = SX(16);
    const int COL_GAP = SX(16);
    const int GROUP_H = SX(kGroupTitleH);
    const int GROUP_GAP = SX(kGroupGapBelowTitle);
    const int NEXT_GRP = SX(kGapToNextGroup);
    const int labelOffset = (FIELD_H - SX(20)) / 2; // label vertical align
    // ---- 탭 컨트롤 위치 ----
    // 탭 바를 타이틀/서브타이틀 구분선 바로 아래 배치
    const int TAB_INSET = SX(2); // keep tab visuals from touching outer card border
    int tabLeft = SX(20) + TAB_INSET;
    int tabRight = rc.Width() - SX(20) - TAB_INSET;
    int tabTop = SX(kTabBarTop);
    int tabBottom = tabTop + SX(kTabBarH) + SX(200); // 탭 컨트롤 전체 높이(내부 클라이언트 포함)
    int tabH = SX(CModernTabCtrl::kBarH) + SX(8); // 탭 바 높이 + 여백
    m_tabCtrl.MoveWindow(tabLeft, tabTop, tabRight - tabLeft, tabH);
    // 탭 컨텐츠 영역: 탭 바 바로 아래부터
    m_rcTabContent = CRect(tabLeft, tabTop + tabH, tabRight, rc.bottom - SX(90));
    // ---- 컨텐츠 영역 기준 좌표 ----
    int contentLeft = m_rcTabContent.left + (MARGIN - tabLeft);
    int x = max(contentLeft, MARGIN);
    int x1 = x;
    int x2 = x1 + LABEL_W + FIELD_W + COL_GAP;
    int x3 = x2 + LABEL_W + FIELD_W + COL_GAP;
    // 모든 탭의 컨텐츠는 동일한 Y 기준에서 배치 (ShowTab이 show/hide)
    int y = m_rcTabContent.top + SX(kTabPadTop);
    auto Move = [&](int id, int mx, int my, int mw, int mh)
        {
            CWnd* p = GetDlgItem(id);
            if (!p || !p->GetSafeHwnd()) return;
            // ComboBox: the height parameter also affects the drop-list height.
            // Keep selection-field height via CB_SETITEMHEIGHT, but preserve a usable drop height.
            TCHAR cls[64] = { 0 };
            ::GetClassName(p->GetSafeHwnd(), cls, 63);
            if (_tcsicmp(cls, _T("ComboBox")) == 0)
            {
                p->MoveWindow(mx, my, mw, COMBO_DROP_H);
                // -1: selection field, 0: list items → CTRL_H로 Edit와 동일하게 맞춤
                ::SendMessage(p->GetSafeHwnd(), CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)(CTRL_H - 2));
                ::SendMessage(p->GetSafeHwnd(), CB_SETITEMHEIGHT, (WPARAM)0, (LPARAM)(CTRL_H - 2));
            }
            else
            {
                p->MoveWindow(mx, my, mw, CTRL_H);
            }
            // ShowTab will show/hide per active tab
            p->ShowWindow(SW_HIDE);
        };
    // =================================================================
    // Tab 0: 결제 설정 (카드형 레이아웃)
    //   - [서버 설정] : 금융결제원 서버 / 포트번호
    //   - [결제 방식] : 통신방식 / 우선 거래 설정 / 무서명 기준금액 / 세금 자동 역산 / 현금영수증
    // =================================================================
    ApplyLayoutTab0();
    // =================================================================
    // Tab 1/2 공통 카드 레이아웃 파라미터 [TUNE]
    // =================================================================
    ApplyLayoutTab1();
    // =================================================================
    // Tab 3: 가맹점 다운로드  (카드 헤더/세로선/배경은 ShopSetupDlg::OnPaint에서 DrawMinCard로 그림)
    // =================================================================
    ApplyLayoutTab3();
    // =================================================================
    // 하단 버튼 (탭과 무관하게 항상 표시)
    // =================================================================
    {
        const int BUTTON_H = 36;    // [TUNE] 버튼 높이
        const int BUTTON_BOTTOM = 18;   // [TUNE] 버튼 하단 여백
        const int BUTTON_GAP = 8;    // [TUNE] 버튼 간격
        const int BUTTON_W = 110;  // [TUNE] 버튼 폭
        // btnY: 다이얼로그 하단에서 역산 (CARD_PAD=메인카드 하단 여백 포함)
        int btnY = rc.bottom - (22 + BUTTON_BOTTOM + BUTTON_H);
        int btnX = rc.Width() / 2;
        m_btnOk.MoveWindow(btnX - BUTTON_W - BUTTON_GAP / 2, btnY, BUTTON_W, BUTTON_H);
        m_btnCancel.MoveWindow(btnX + BUTTON_GAP / 2, btnY, BUTTON_W, BUTTON_H);
        m_btnOk.ShowWindow(SW_SHOW);
        m_btnCancel.ShowWindow(SW_SHOW);
    }
}
// --------------------------------------------------------------
// 레지스트리 → UI
//  - 저장된 값이 없으면 기본값을 사용
//  - 콤보박스는 '표시 문자열'과 '실제 저장 값'을 구분해서 매핑
//
// [초기값 확인 방법]
//  - 이 함수 안의 각 if (GetRegisterData(...)) else 블록을 보면 된다.
//  - else 쪽에 대입되는 값이 "레지스트리에 값이 없을 때의 초기값"이다.
//  - 예시
//      m_intPort           = 8002
//      m_intTaxPercent     = 0
//      m_intCardTimeout    = 100
//      m_intNoSignAmount   = 50000
//      m_intSignPadPort    = 0
//      m_intScannerPort    = 0
//      m_strCardDetectParam= ""
//  - 토글/콤보 기본값은 아래쪽 ReadToggle_DefaultOnWhenMissing(),
//    SelectComboByValue(..., 기본값, defaultIndex) 호출부에서 확인 가능하다.
// --------------------------------------------------------------
void CShopSetupDlg::LoadOptionsFromRegistry()
{
    /* [UI-STEP] 레지스트리 → UI 로드(기본값/변환 포함)
     * 1) 각 옵션 키(섹션/이름)로 레지스트리 값을 읽는다(없으면 기본값 적용).
     * 2) 문자열 → int/bool/enum 변환을 수행하고 범위를 체크한다.
     * 3) 변환된 값을 콤보 선택/에딧 텍스트/토글 상태로 UI에 반영한다.
     * 4) 토글 상태에 따라 종속 옵션(서명패드 속도 등)의 EnableWindow를 조정한다.
     *
     * [참고]
     * - 변환 실패 시(빈 문자열/비정상 값) 크래시 없이 기본값으로 폴백하는 것이 안정적이다.
     */
    CString s;
    // -------------------------
    // TCP
    // -------------------------
    if (GetRegisterData(SEC_TCP, VAN_SERVER_IP_FIELD, s))
        SelectComboByValue(m_comboVanServer, kVanServers, (int)(sizeof(kVanServers) / sizeof(kVanServers[0])), s, 0);
    else
        SelectComboByValue(m_comboVanServer, kVanServers, (int)(sizeof(kVanServers) / sizeof(kVanServers[0])), _T("www.kftcvan.or.kr"), 0); // 기본: 운영 서버
    if (GetRegisterData(SEC_TCP, VAN_SERVER_PORT_FIELD, s))
        m_intPort = _ttoi(s);
    else
        m_intPort = 8002; // 기본값
    if (GetRegisterData(SEC_TCP, TAX_SETTING_FIELD, s))
        m_intTaxPercent = _ttoi(s);
    else
        m_intTaxPercent = 10; // 기본값
    // -------------------------
    // SERIALPORT
    // -------------------------
    if (GetRegisterData(SEC_SERIALPORT, TIMEOUT_FIELD, s))
        m_intCardTimeout = _ttoi(s);
    else
        m_intCardTimeout = 100; // 기본값
    if (GetRegisterData(SEC_SERIALPORT, NOSIGN_AMT_FIELD, s))
        m_intNoSignAmount = _ttoi(s);
    else
        m_intNoSignAmount = 50000; // 기본값
    if (GetRegisterData(SEC_SERIALPORT, SIGNPAD_FIELD, s))
        m_intSignPadPort = _ttoi(s);
    else
        m_intSignPadPort = 0; // 기본값
    if (GetRegisterData(SEC_SERIALPORT, BARCODE_PORT_FIELD, s))
        m_intScannerPort = _ttoi(s);
    else
        m_intScannerPort = 0; // 기본값
    if (GetRegisterData(SEC_SERIALPORT, DETECT_PROGRAM_FIELD, s))
        m_strCardDetectParam = s;
    else
        m_strCardDetectParam = _T(""); // 기본값
    // Combo: 현금영수증 거래 (기본: PINPAD/KEYIN)
    if (GetRegisterData(SEC_SERIALPORT, CASH_FIRST_FIELD, s))
        SelectComboByValue(m_comboCashReceipt, kCashReceipt, (int)(sizeof(kCashReceipt) / sizeof(kCashReceipt[0])), s, 0);
    else
        SelectComboByValue(m_comboCashReceipt, kCashReceipt, (int)(sizeof(kCashReceipt) / sizeof(kCashReceipt[0])), _T("PINPAD/KEYIN"), 0);
    // Combo: 장치 연동 방식 (기본: NORMAL)
    if (GetRegisterData(SEC_SERIALPORT, INTERLOCK_FIELD, s))
        SelectComboByValue(m_comboInterlock, kInterlock, (int)(sizeof(kInterlock) / sizeof(kInterlock[0])), s, 0);
    else
        SelectComboByValue(m_comboInterlock, kInterlock, (int)(sizeof(kInterlock) / sizeof(kInterlock[0])), _T("NORMAL"), 0);
    // Combo: 통신방식 (기본: CS 방식)
    if (GetRegisterData(SEC_SERIALPORT, SOCKET_TYPE_FIELD, s))
        SelectComboByValue(m_comboCommType, kCommType, (int)(sizeof(kCommType) / sizeof(kCommType[0])), s, 0);
    else
        SelectComboByValue(m_comboCommType, kCommType, (int)(sizeof(kCommType) / sizeof(kCommType[0])), _T("CS 방식"), 0);
    // Combo: 서명패드 사용 (기본: YES)
    if (GetRegisterData(SEC_SERIALPORT, SIGNPAD_USE_FIELD, s))
        SelectComboByValue(m_comboSignPadUse, kSignPadUse, (int)(sizeof(kSignPadUse) / sizeof(kSignPadUse[0])), s, 0);
    else
        SelectComboByValue(m_comboSignPadUse, kSignPadUse, (int)(sizeof(kSignPadUse) / sizeof(kSignPadUse[0])), _T("YES"), 0);
    // Combo: 서명패드 속도 (기본: 57600)
    if (GetRegisterData(SEC_SERIALPORT, SIGNPAD_SPEED_FIELD, s))
        SelectComboByValue(m_comboSignPadSpeed, kSignPadSpeed, (int)(sizeof(kSignPadSpeed) / sizeof(kSignPadSpeed[0])), s, 0);
    else
        SelectComboByValue(m_comboSignPadSpeed, kSignPadSpeed, (int)(sizeof(kSignPadSpeed) / sizeof(kSignPadSpeed[0])), _T("57600"), 0);
    // Combo: 알림창 표시 위치 (기본: mid)
    if (GetRegisterData(SEC_SERIALPORT, NOTIFY_POS_FIELD, s))
        SelectComboByValue(m_comboAlarmPos, kAlarmPos, (int)(sizeof(kAlarmPos) / sizeof(kAlarmPos[0])), s, 1);
    else
        SelectComboByValue(m_comboAlarmPos, kAlarmPos, (int)(sizeof(kAlarmPos) / sizeof(kAlarmPos[0])), _T("mid"), 1);
    // Combo: 알림창 크기 (기본: verysmall)
    if (GetRegisterData(SEC_SERIALPORT, NOTIFY_SIZE_FIELD, s))
        SelectComboByValue(m_comboAlarmSize, kAlarmSize, (int)(sizeof(kAlarmSize) / sizeof(kAlarmSize[0])), s, 1);
    else
        SelectComboByValue(m_comboAlarmSize, kAlarmSize, (int)(sizeof(kAlarmSize) / sizeof(kAlarmSize[0])), _T("verysmall"), 1);
    // Combo: 단축키 (요청취소/ MSR 전환) 기본: NORMAL
    if (GetRegisterData(SEC_SERIALPORT, CANCEL_HOTKEY_FIELD, s))
        SelectComboByValue(m_comboCancelKey, kHotkeys, (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])), s, 0);
    else
        SelectComboByValue(m_comboCancelKey, kHotkeys, (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])), _T("NORMAL"), 0);
    if (GetRegisterData(SEC_SERIALPORT, MSR_HOTKEY_FIELD, s))
        SelectComboByValue(m_comboMSRKey, kHotkeys, (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])), s, 0);
    else
        SelectComboByValue(m_comboMSRKey, kHotkeys, (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])), _T("NORMAL"), 0);
    // Toggle 기본값(밑줄) 반영:
    // - 우선 거래(CARD_DETECT): OFF(0)
    // - 멀티패드 음성 출력(MULTIPAD_SOUND): OFF(0)
    // - 스캐너 사용(BARCODE_USE): OFF(0)
    // - 알림창 그림(NOTIFY_IMG): ON(0)
    // - 알림창 듀얼(NOTIFY_DUAL_MONITOR): OFF(0)
    // - 자동 재실행(AUTO_RESTART): ON(0)
    // - 자동 리부팅(AUTO_REBOOT): ON(0)
    m_chkCardDetect.SetToggled(ReadToggle_DefaultOnWhenMissing(CARD_DETECT_FIELD, FALSE, _T("0"), _T("1")));
    m_chkMultiVoice.SetToggled(ReadToggle_DefaultOnWhenMissing(MULTIPAD_SOUND_FIELD, FALSE, _T("1"), _T("0")));
    // BARCODE_USE: OFF=0 / ON=1
    m_chkScannerUse.SetToggled(ReadToggle_DefaultOnWhenMissing(BARCODE_USE_FIELD, FALSE, _T("1"), _T("0")));
    // NOTIFY_IMG: ON=0 / OFF=1
    m_chkAlarmGraph.SetToggled(ReadToggle_DefaultOnWhenMissing(NOTIFY_IMG_FIELD, TRUE, _T("0"), _T("1")));
    // NOTIFY_DUAL_MONITOR: ON=1 / OFF=0
    m_chkAlarmDual.SetToggled(ReadToggle_DefaultOnWhenMissing(NOTIFY_DUAL_FIELD, FALSE, _T("1"), _T("0")));
    // AUTO_*: ON=0 / OFF=1
    m_chkAutoReboot.SetToggled(ReadToggle_DefaultOnWhenMissing(AUTO_REBOOT_FIELD, TRUE, _T("0"), _T("1")));
    // UI에 반영
    UpdateData(FALSE);
    UpdateToggleDependentEdits(FALSE);
    TakeSnapshot();
}
// ============================================================================
// SaveOptionsToRegistry - OK 버튼에서 일괄 저장
// ============================================================================
// --------------------------------------------------------------
// UI → 레지스트리
//  - 현재 컨트롤 상태(콤보 선택/에딧 텍스트/토글 ON-OFF)를 읽어서 저장
//  - 값 검증(숫자 범위, 빈 값 처리 등)이 필요하면 여기에서 일괄 적용 권장
// --------------------------------------------------------------
// --------------------------------------------------------------
// UI → 레지스트리 저장
//  - OnOK()에서 ValidateAllInputs() 통과 후에만 호출된다.
//  - 따라서 이 함수는 "검증이 끝난 정상값"을 저장하는 단계라고 보면 된다.
// --------------------------------------------------------------
void CShopSetupDlg::GetVanSettings(CString& strIp, CString& strPort)
{
    strIp = GetSelectedComboValue(m_comboVanServer, kVanServers,
                                  (int)(sizeof(kVanServers) / sizeof(kVanServers[0])),
                                  _T("www.kftcvan.or.kr"));
    strPort.Empty();
    if (m_editPort.GetSafeHwnd())
        m_editPort.GetWindowText(strPort);
    if (strPort.IsEmpty())
        strPort = _T("443");
}

void CShopSetupDlg::SaveOptionsToRegistry()
{
    UpdateData(TRUE);
    // TCP
    {
        CString v = GetSelectedComboValue(m_comboVanServer, kVanServers, (int)(sizeof(kVanServers) / sizeof(kVanServers[0])), _T("www.kftcvan.or.kr"));
        AfxGetApp()->WriteProfileString(SEC_TCP, VAN_SERVER_IP_FIELD, v);
        CString s; s.Format(_T("%d"), m_intPort);
        AfxGetApp()->WriteProfileString(SEC_TCP, VAN_SERVER_PORT_FIELD, s);
        CString t; t.Format(_T("%d"), m_intTaxPercent);
        AfxGetApp()->WriteProfileString(SEC_TCP, TAX_SETTING_FIELD, t);
    }
    // SERIALPORT - Edit
    {
        CString s;
        s.Format(_T("%d"), m_intCardTimeout);
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, TIMEOUT_FIELD, s);
        s.Format(_T("%d"), m_intNoSignAmount);
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, NOSIGN_AMT_FIELD, s);
        s.Format(_T("%d"), m_intSignPadPort);
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, SIGNPAD_FIELD, s);
        s.Format(_T("%d"), m_intScannerPort);
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, BARCODE_PORT_FIELD, s);
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, DETECT_PROGRAM_FIELD, m_strCardDetectParam);
    }
    // SERIALPORT - Combo
    {
        CString v;
        v = GetSelectedComboValue(m_comboCashReceipt, kCashReceipt, (int)(sizeof(kCashReceipt) / sizeof(kCashReceipt[0])), _T("PINPAD/KEYIN"));
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, CASH_FIRST_FIELD, v);
        v = GetSelectedComboValue(m_comboInterlock, kInterlock, (int)(sizeof(kInterlock) / sizeof(kInterlock[0])), _T("NORMAL"));
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, INTERLOCK_FIELD, v);
        v = GetSelectedComboValue(m_comboCommType, kCommType, (int)(sizeof(kCommType) / sizeof(kCommType[0])), _T("CS 방식"));
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, SOCKET_TYPE_FIELD, v);
        v = GetSelectedComboValue(m_comboSignPadUse, kSignPadUse, (int)(sizeof(kSignPadUse) / sizeof(kSignPadUse[0])), _T("YES"));
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, SIGNPAD_USE_FIELD, v);
        v = GetSelectedComboValue(m_comboSignPadSpeed, kSignPadSpeed, (int)(sizeof(kSignPadSpeed) / sizeof(kSignPadSpeed[0])), _T("57600"));
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, SIGNPAD_SPEED_FIELD, v);
        v = GetSelectedComboValue(m_comboAlarmPos, kAlarmPos, (int)(sizeof(kAlarmPos) / sizeof(kAlarmPos[0])), _T("mid"));
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, NOTIFY_POS_FIELD, v);
        v = GetSelectedComboValue(m_comboAlarmSize, kAlarmSize, (int)(sizeof(kAlarmSize) / sizeof(kAlarmSize[0])), _T("verysmall"));
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, NOTIFY_SIZE_FIELD, v);
        v = GetSelectedComboValue(m_comboCancelKey, kHotkeys, (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])), _T("NORMAL"));
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, CANCEL_HOTKEY_FIELD, v);
        v = GetSelectedComboValue(m_comboMSRKey, kHotkeys, (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])), _T("NORMAL"));
        AfxGetApp()->WriteProfileString(SEC_SERIALPORT, MSR_HOTKEY_FIELD, v);
    }
    // SERIALPORT - Toggle
    WriteToggleValue(CARD_DETECT_FIELD, m_chkCardDetect.IsToggled(), _T("0"), _T("1")); // ON=0, OFF=1
    WriteToggleValue(MULTIPAD_SOUND_FIELD, m_chkMultiVoice.IsToggled(), _T("1"), _T("0")); // ON=1, OFF=0
    WriteToggleValue(BARCODE_USE_FIELD, m_chkScannerUse.IsToggled(), _T("1"), _T("0")); // ON=1, OFF=0
    WriteToggleValue(NOTIFY_DUAL_FIELD, m_chkAlarmDual.IsToggled(), _T("1"), _T("0")); // ON=1, OFF=0
    // NOTIFY_IMG: ON=0, OFF=1
    AfxGetApp()->WriteProfileString(SEC_SERIALPORT, NOTIFY_IMG_FIELD, m_chkAlarmGraph.IsToggled() ? _T("0") : _T("1"));
    // AUTO_*: ON=0, OFF=1
    AfxGetApp()->WriteProfileString(SEC_SERIALPORT, AUTO_REBOOT_FIELD, m_chkAutoReboot.IsToggled() ? _T("0") : _T("1"));
}
// ============================================================================
// ShowTab - 탭 전환 시 컨트롤 show/hide
// ============================================================================
void CShopSetupDlg::ShowTab(int nTab)
{
    if (m_bClosing || !GetSafeHwnd()) return;
    m_nActiveTab = nTab;
    if (m_popover.GetSafeHwnd()) m_popover.Hide();
    // [1] 그리기 잠금: 수많은 컨트롤의 Show/Hide 시 발생하는 노이즈 차단
    this->SendMessage(WM_SETREDRAW, FALSE);
    // 각 탭별 컨트롤 ID 정의 (기존 리스트 유지)
    static const int s_tab0[] = {
        IDC_STATIC_VAN_SERVER, IDC_COMBO_VAN_SERVER, IDC_STATIC_PORT, IDC_EDIT_PORT, IDC_STATIC_ERR_PORT,
        IDC_STATIC_COMM_TYPE, IDC_COMBO_COMM_TYPE, IDC_STATIC_CASH_RECEIPT, IDC_COMBO_CASH_RECEIPT,
        IDC_CHECK_CARD_DETECT, IDC_STATIC_CARD_DETECT_POSINFO, IDC_EDIT_CARD_DETECT_PARAM, IDC_STATIC_ERR_CARD_DETECT_PROGRAM,
        IDC_STATIC_NO_SIGN_AMOUNT, IDC_EDIT_NO_SIGN_AMOUNT, IDC_STATIC_ERR_NO_SIGN,
        IDC_STATIC_TAX_PERCENT, IDC_EDIT_TAX_PERCENT, IDC_STATIC_ERR_TAX, 0
    };
    static const int s_tab1[] = {
        IDC_STATIC_CARD_TIMEOUT, IDC_EDIT_CARD_TIMEOUT, IDC_STATIC_ERR_TIMEOUT, IDC_STATIC_INTERLOCK, IDC_COMBO_INTERLOCK,
        IDC_STATIC_SIGN_PAD_USE, IDC_COMBO_SIGN_PAD_USE, IDC_STATIC_SIGN_PAD_PORT, IDC_EDIT_SIGN_PAD_PORT, IDC_STATIC_ERR_SIGNPAD_PORT,
        IDC_STATIC_SIGN_PAD_SPEED, IDC_COMBO_SIGN_PAD_SPEED, IDC_CHECK_SCANNER_USE, IDC_STATIC_SCANNER_PORT_LABEL,
        IDC_EDIT_SCANNER_PORT, IDC_STATIC_ERR_SCANNER_PORT, IDC_CHECK_MULTI_VOICE, 0
    };
    static const int s_tab2[] = {
        IDC_STATIC_ALARM_SIZE, IDC_COMBO_ALARM_SIZE, IDC_STATIC_ALARM_POS, IDC_COMBO_ALARM_POS,
        IDC_CHECK_ALARM_GRAPH, IDC_CHECK_ALARM_DUAL, IDC_CHECK_AUTO_REBOOT,
        IDC_STATIC_CANCEL_KEY, IDC_COMBO_CANCEL_KEY, IDC_STATIC_MSR_KEY, IDC_COMBO_MSR_KEY, 0
    };
    const int* tabs[3] = { s_tab0, s_tab1, s_tab2 };
    // [2] 모든 탭 전용 컨트롤 일괄 숨기기
    for (int t = 0; t < 3; t++) {
        for (int i = 0; tabs[t][i]; i++) {
            CWnd* p = GetDlgItem(tabs[t][i]);
            if (p && p->GetSafeHwnd()) p->ShowWindow(SW_HIDE);
        }
    }
    // [3] 탭 3(가맹점 다운로드) 처리
    if (nTab == 3) {
        if (m_staticShopContainer.GetSafeHwnd()) {
            // ★ 깜빡임 방지 핵심: 자식 영역을 그리기에서 제외
            m_staticShopContainer.ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
            CRect rcHost;
            m_staticShopContainer.GetClientRect(&rcHost);
            // [FIX] Position only - no SWP_SHOWWINDOW here. Showing happens after
            // SetRedraw(TRUE) so DWM sees a single clean composition frame.
            m_shopDownDlg.SetWindowPos(NULL, 0, 0, rcHost.Width(), rcHost.Height(), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
            // 자식 창의 레이아웃 재계산 강제 트리거
            m_shopDownDlg.SendMessage(WM_SIZE, 0, MAKELPARAM(rcHost.Width(), rcHost.Height()));
        }
    }
    else {
        if (m_staticShopContainer.GetSafeHwnd()) m_staticShopContainer.ShowWindow(SW_HIDE);
        // 일반 탭 컨트롤 표시
        if (nTab >= 0 && nTab < 3) {
            for (int i = 0; tabs[nTab][i]; i++) {
                CWnd* p = GetDlgItem(tabs[nTab][i]);
                if (p && p->GetSafeHwnd()) p->ShowWindow(SW_SHOW);
            }
        }
    }
    // [4] 정보(i) 버튼들 탭에 맞춰 가시성 조정
    m_btnVanInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    m_btnPortInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    m_btnTaxPercentInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    m_btnCommTypeInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    m_btnCashReceiptInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    m_btnCardDetectInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    m_btnCardTimeoutInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    m_btnInterlockInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    m_btnMultiVoiceInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    m_btnScannerUseInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    m_btnSignPadUseInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    m_btnSignPadPortInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    m_btnSignPadSpeedInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    m_btnAlarmSizeInfo.ShowWindow(nTab == 2 ? SW_SHOW : SW_HIDE);
    m_btnAlarmGraphInfo.ShowWindow(nTab == 2 ? SW_SHOW : SW_HIDE);
    m_btnAlarmDualInfo.ShowWindow(nTab == 2 ? SW_SHOW : SW_HIDE);
    m_btnAutoRebootInfo.ShowWindow(nTab == 2 ? SW_SHOW : SW_HIDE);
    RefreshValidationVisibilityByTab();
    // [5] 잠금 해제 및 부모/자식 전체 부드럽게 갱신
    this->SendMessage(WM_SETREDRAW, TRUE);
    // [FIX] Show container + ShopDownDlg AFTER SetRedraw(TRUE).
    // SWP_NOREDRAW marks them visible in the window hierarchy without triggering a
    // DWM composition update. The single RedrawWindow below then paints everything
    // in one frame, eliminating the intermediate-state flicker on external windows.
    if (nTab == 3 && m_staticShopContainer.GetSafeHwnd() && m_shopDownDlg.GetSafeHwnd()) {
        m_staticShopContainer.SetWindowPos(NULL, 0, 0, 0, 0,
            SWP_NOREDRAW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
        m_shopDownDlg.SetWindowPos(NULL, 0, 0, 0, 0,
            SWP_NOREDRAW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
    this->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}
// ============================================================================
// TCN_SELCHANGE 핸들러
// ============================================================================
void CShopSetupDlg::OnTcnSelchange(NMHDR* pNMHDR, LRESULT* pResult)
{
    /* [UI-STEP] 탭 변경 처리(레이아웃 재배치 + 화면 갱신)
     * 1) 현재 탭 인덱스를 갱신한다.
     * 2) ApplyLayout()를 호출해 탭별 컨트롤 배치를 다시 수행한다.
     * 3) 필요한 경우 탭별로 보이기/숨기기(ShowWindow) 처리 후 Invalidate()한다.
     */
    int nSel = m_tabCtrl.GetCurSel();
    if (nSel >= 0)
        ShowTab(nSel);
    *pResult = 0;
}
// ============================================================================
// OnCommand (포커스/드롭다운 리페인트 최소화)
// ============================================================================
BOOL CShopSetupDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    /* [UI-STEP] 컨트롤 이벤트 라우팅(콤보 선택/에딧 변경 등)
     * 1) HIWORD(wParam)로 통지 코드(CBN_SELCHANGE, EN_CHANGE 등)를 판별한다.
     * 2) 해당 컨트롤의 변경을 내부 변수/상태에 반영한다.
     * 3) 변경에 따라 종속 컨트롤 Enable/Disable 또는 재그림이 필요하면 Invalidate()한다.
     */
    const UINT code = HIWORD(wParam);
    switch (code)
    {
    case EN_SETFOCUS: case EN_KILLFOCUS:
    case CBN_SETFOCUS: case CBN_KILLFOCUS:
    case CBN_DROPDOWN: case CBN_CLOSEUP: case CBN_SELCHANGE:
        if (lParam)
        {
            CRect rcCtl;
            ::GetWindowRect((HWND)lParam, &rcCtl);
            ScreenToClient(&rcCtl);
            rcCtl.InflateRect(4, 4);
            RedrawWindow(&rcCtl, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
            if (code == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBO_SIGN_PAD_USE)
                UpdateToggleDependentEdits(TRUE);
        }
        break;
    case EN_CHANGE:
        break;
    case BN_CLICKED:
        break;
    default: break;
    }
    return CDialog::OnCommand(wParam, lParam);
}
// ============================================================================
// OnEraseBkgnd
// ============================================================================
BOOL CShopSetupDlg::OnEraseBkgnd(CDC* pDC)
{
    /* [UI-STEP] 배경 지우기(깜빡임 감소)
     * 1) 배경을 OnPaint에서 전부 그리는 구조면 여기서는 TRUE를 리턴해 기본 지우기를 막는다.
     * 2) 기본 지우기를 막을 때는 반드시 OnPaint에서 전체 배경을 빠짐없이 칠해야 잔상이 남지 않는다.
     */
    return TRUE;
}
// ============================================================================
// OnLButtonDown
// ============================================================================
void CShopSetupDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    CDialog::OnLButtonDown(nFlags, point);
    CWnd* pChild = ChildWindowFromPoint(point, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
    if (pChild == NULL || pChild == this) { SetFocus(); return; }
    TCHAR cls[64] = { 0 };
    ::GetClassName(pChild->GetSafeHwnd(), cls, 63);
    if (_tcsicmp(cls, _T("Edit")) != 0 &&
        _tcsicmp(cls, _T("ComboBox")) != 0 &&
        _tcsicmp(cls, _T("Button")) != 0 &&
        _tcsicmp(cls, _T("SysTabControl32")) != 0)
        SetFocus();
}
// ============================================================================
// OnPaint - 더블버퍼링, 타이틀 + 그룹 소제목 그리기
// ============================================================================
// --------------------------------------------------------------
// 커스텀 페인팅
//  - 배경/섹션 카드/라벨 등 정적 UI를 GDI+/GDI로 직접 그림
//  - 인풋 보더는 별도 함수(DrawInputBorders 등)에서 공통 처리
// --------------------------------------------------------------
void CShopSetupDlg::OnPaint()
{
    /* [UI-STEP] 커스텀 페인팅(배경/라벨/입력 보더) 렌더링
     * 1) CPaintDC로 paint DC를 얻고(필요 시 메모리 DC로 더블버퍼) 깜빡임을 줄인다.
     * 2) DrawBackground()로 전체 배경(카드/섹션 배경 포함)을 그린다.
     * 3) DrawGroupLabels()로 그룹 타이틀/라벨 텍스트를 그린다(폰트/색 위계 적용).
     * 4) DrawInputBorders()로 Edit/Combo 주변 보더를 일괄로 그린다(포커스/hover 상태 반영).
     * 5) 필요하면 DrawSectionIcon() 등 아이콘/장식 요소를 마지막에 그려 z-order 느낌을 맞춘다.
     *
     * [참고]
     * - 배경을 직접 그리는 경우 OnEraseBkgnd에서 TRUE 리턴으로 깜빡임을 줄이는 패턴을 함께 쓴다.
     * - 컨트롤 자체가 그리는 영역과 겹치면(클리핑) 테두리 잔상 문제가 생길 수 있어 그리기 순서가 중요하다.
     */
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(&rc);
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);
    // [추가] 여기서 배경을 먼저 한 번만 채웁니다.
    memDC.FillSolidRect(rc, RGB(249, 250, 252));
    DrawBackground(&memDC);
    // ── 헤더: 배지 아이콘 + 타이틀 + 서브타이틀 ───────────────────
    {
        ModernUIHeader::Draw(memDC.GetSafeHdc(),
            (float)(m_rcOuterCard.left + SX(14)), (float)(m_rcOuterCard.top + SX(16)), (float)SX(kHdrBadgeSz),
            ModernUIHeader::IconType::Store,
            L"가맹점 설정", L"가맹점 및 서버 연결 설정을 관리합니다",
            m_hFontHdrTitle, m_hFontHdrSub,
            m_rcOuterCard.left + SX(14), m_rcOuterCard.top + SX(74), rc.Width() - (m_rcOuterCard.left + SX(14)));
    }
    CFont* pOldFont = memDC.SelectObject(&m_fontTitle);
    CPen linePen(PS_SOLID, 1, RGB(228, 232, 240));
    CPen* pOldPen = memDC.SelectObject(&linePen);
    // 그룹 소제목 (활성 탭에 따라)
    DrawGroupLabels(&memDC);
    memDC.SelectObject(pOldPen);
    memDC.SelectObject(pOldFont);
    // Save-success toast overlay
    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}
// ============================================================================
// DrawGroupLabels - 활성 탭의 그룹 소제목([결제 방식] 등) 그리기
// ============================================================================
void CShopSetupDlg::DrawGroupLabels(CDC* pDC)
{
    /* [UI-STEP] 그룹/라벨 텍스트 그리기(가독성/정렬 규칙)
     * 1) 각 그룹의 타이틀 위치를 계산하고 제목 폰트로 출력한다.
     * 2) 각 행 라벨은 동일한 X 기준으로 정렬해 UI 리듬을 만든다.
     * 3) 비활성 상태 컨트롤은 라벨 색을 약하게 처리할 수 있다.
     */
    CFont* pOld = m_fontGroupTitle.GetSafeHandle()
        ? pDC->SelectObject(&m_fontGroupTitle) : nullptr;
    pDC->SetBkMode(TRANSPARENT);
    auto DrawGrp = [&](const CRect& r, LPCTSTR t)
        {
            if (r.IsRectEmpty()) return;
            ModernUIGfx::EnsureGdiplusStartup();
            HDC hdc = pDC->GetSafeHdc();
            Gdiplus::Graphics gfx(hdc);
            gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            // 왼쪽 컬러 바 (BLUE_400, 3px 너비, 둥근 모서리)
            Gdiplus::SolidBrush barBrush(Gdiplus::Color(255, 15, 124, 255));
            Gdiplus::RectF barRf((Gdiplus::REAL)r.left, (Gdiplus::REAL)r.top + 3.0f,
                3.0f, (Gdiplus::REAL)(r.Height() - 6));
            gfx.FillRectangle(&barBrush, barRf);
            // 텍스트: BLUE_600 색
            pDC->SetTextColor(RGB(0, 76, 168));  // BLUE_600
            CRect rcTxt = r;
            rcTxt.left += 9;
            pDC->DrawText(t, (LPRECT)&rcTxt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        };
    // Tab1/2 헤더는 DrawBackground의 DrawMinCard 람다에서 처리
    // (이중 렌더링 방지를 위해 여기서는 그리지 않음)
    switch (m_nActiveTab)
    {
    case 1: break;
    case 2: break;
    default: break;
    }
    if (pOld) pDC->SelectObject(pOld);
}
// ============================================================================
// DrawBackground
// ============================================================================
void CShopSetupDlg::DrawBackground(CDC* pDC)
{
    /* [UI-STEP] 배경 그리기(앱 배경 + 카드형 영역)
     * 1) 클라이언트 전체를 기본 배경색으로 채운다.
     * 2) 섹션/그룹 영역을 카드 형태(라운드, 그림자/보더)로 분리해 그린다.
     * 3) 선택 탭에 따라 표시할 영역만 강조/그리도록 분기할 수 있다.
     */
    CRect rc;
    GetClientRect(&rc);
    //pDC->FillSolidRect(rc, RGB(249, 250, 252));  // 밝은 회색 배경
    const int kCardMarginL = 20;
    const int kCardMarginT = 10;
    const int kCardMarginR = 20;
    const int kCardMarginB = 20;
    const float kRadius = 12.0f;
    CRect contentRect(kCardMarginL, kCardMarginT,
        rc.Width() - kCardMarginR, rc.bottom - kCardMarginB);
    m_rcOuterCard = contentRect;
    ModernUIGfx::EnsureGdiplusStartup();
    {
        HDC hdc = pDC->GetSafeHdc();
        Gdiplus::Graphics g(hdc);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        const Gdiplus::RectF rf(
            (Gdiplus::REAL)contentRect.left,
            (Gdiplus::REAL)contentRect.top,
            (Gdiplus::REAL)contentRect.Width(),
            (Gdiplus::REAL)contentRect.Height());
        auto AddRoundRectPath = [](Gdiplus::GraphicsPath& path,
            const Gdiplus::RectF& r, float radius)
            {
                const float d = radius * 2.0f;
                if (radius <= 0.0f) { path.AddRectangle(r); return; }
                Gdiplus::RectF arc(r.X, r.Y, d, d);
                path.AddArc(arc, 180, 90);
                arc.X = r.X + r.Width - d;
                path.AddArc(arc, 270, 90);
                arc.Y = r.Y + r.Height - d;
                path.AddArc(arc, 0, 90);
                arc.X = r.X;
                path.AddArc(arc, 90, 90);
                path.CloseFigure();
            };
        Gdiplus::GraphicsPath path;
        AddRoundRectPath(path, rf, kRadius);
        // 메인카드 그림자 3단계
        for (int sh = 3; sh >= 1; sh--)
        {
            Gdiplus::RectF shRf(rf.X, rf.Y + (float)sh, rf.Width, rf.Height);
            Gdiplus::GraphicsPath shPath;
            AddRoundRectPath(shPath, shRf, kRadius);
            BYTE alpha = (BYTE)(sh == 3 ? 8 : sh == 2 ? 14 : 20);
            Gdiplus::SolidBrush shBrush(Gdiplus::Color(alpha, 10, 30, 70));
            g.FillPath(&shBrush, &shPath);
        }
        Gdiplus::SolidBrush fillBrush(Gdiplus::Color(255, 255, 255, 255));
        g.FillPath(&fillBrush, &path);
        Gdiplus::Pen borderPen(Gdiplus::Color(255, 220, 224, 234), 1.0f);
        borderPen.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawPath(&borderPen, &path);
        // ================================================================
        // 공용 헬퍼: RR + DrawMinCard (Tab 0/1/2 공통 사용)
        // ================================================================
        auto RR = [](Gdiplus::GraphicsPath& path, const Gdiplus::RectF& r, float rad)
            {
                const float d = rad * 2.0f;
                if (rad <= 0.0f) { path.AddRectangle(r); return; }
                Gdiplus::RectF a(r.X, r.Y, d, d);
                path.AddArc(a, 180, 90); a.X = r.X + r.Width - d;
                path.AddArc(a, 270, 90); a.Y = r.Y + r.Height - d;
                path.AddArc(a, 0, 90); a.X = r.X;
                path.AddArc(a, 90, 90); path.CloseFigure();
            };
        auto DrawMinCard = [&](const CRect& rcSec, const wchar_t* title)
            {
                if (rcSec.IsRectEmpty()) return;
                const float crad = 12.0f;
                const float hdrH = 44.0f;
                Gdiplus::RectF cr(
                    (float)rcSec.left, (float)rcSec.top,
                    (float)rcSec.Width(), (float)rcSec.Height());
                // 그림자를 더 연하게, 레이어는 줄이되 간격을 넓힘
                for (int sh = 1; sh <= 2; sh++) {
                    Gdiplus::RectF sr(cr.X, cr.Y + (float)sh * 1.5f, cr.Width, cr.Height);
                    Gdiplus::GraphicsPath sp; RR(sp, sr, crad);
                    // 투명도를 8(매우 연하게)로 조정하여 '스며드는' 느낌 강조
                    Gdiplus::SolidBrush sb(Gdiplus::Color(8, 0, 0, 0));
                    g.FillPath(&sb, &sp);
                }
                // 카드 본체 흰색
                Gdiplus::GraphicsPath cp; RR(cp, cr, crad);
                Gdiplus::SolidBrush cf(Gdiplus::Color(255, 250, 251, 253));  // 그룹 배경(#FAFBFD)
                g.FillPath(&cf, &cp);
                // 헤더 구분선
                Gdiplus::Pen hl(Gdiplus::Color(255, 238, 241, 247), 1.0f);
                g.DrawLine(&hl,
                    Gdiplus::PointF(cr.X + 16.0f, cr.Y + hdrH),
                    Gdiplus::PointF(cr.X + cr.Width - 16.0f, cr.Y + hdrH));
                // 불릿 + 타이틀
                const float barX = cr.X + 16.0f;       // 카드 좌측 내부 여백
                const float barW = 4.0f;               // 세로 바 폭
                const float barH = 14.0f;              // 세로 바 높이
                const float barY = cr.Y + (hdrH - barH) * 0.5f;  // 헤더 세로 중앙
                const float barR = 2.0f;               // 모서리 라운드 반경
                // 세로 accent bar (라운드 사각형)
                Gdiplus::GraphicsPath barPath;
                const float bd = barR * 2.0f;
                barPath.AddArc(barX, barY, bd, bd, 180, 90);
                barPath.AddArc(barX + barW - bd, barY, bd, bd, 270, 90);
                barPath.AddArc(barX + barW - bd, barY + barH - bd, bd, bd, 0, 90);
                barPath.AddArc(barX, barY + barH - bd, bd, bd, 90, 90);
                barPath.CloseFigure();
                Gdiplus::SolidBrush barBr(Gdiplus::Color(255, 0, 96, 210));
                g.FillPath(&barBr, &barPath);
                // 타이틀 (세로 바 오른쪽에 10px 간격)
                const float titleX = barX + barW + 6.0f;
                // Use cached member font object (created once in OnInitDialog with DPI scaling)
                {
                    HDC hdcCard = g.GetHDC();
                    ::SetBkMode(hdcCard, TRANSPARENT);
                    HFONT hOldCard = (HFONT)::SelectObject(hdcCard, m_hFontCardTitle);
                    ::SetTextColor(hdcCard, RGB(26, 32, 44));
                    RECT rcCard = { (LONG)titleX, (LONG)cr.Y, (LONG)(cr.X + cr.Width - 16.0f), (LONG)(cr.Y + hdrH) };
                    ::DrawTextW(hdcCard, title, -1, &rcCard, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
                    ::SelectObject(hdcCard, hOldCard);
                    g.ReleaseHDC(hdcCard);
                }
            };
        // ── 탭별 렌더링 ────────────────────────────────────────────────
        if (m_nActiveTab == 0)
        {
            DrawMinCard(m_rcCardServer, L"서버 설정");
            DrawMinCard(m_rcCardPayMethod, L"결제 방식");
        }
        else if (m_nActiveTab == 1)
        {
            DrawMinCard(m_rcGrpReader, L"리더기");
            DrawMinCard(m_rcGrpSign, L"서명패드");
            DrawMinCard(m_rcGrpEtc, L"기타");
        }
        else if (m_nActiveTab == 2)
        {
            DrawMinCard(m_rcGrpAlarm, L"알림창 설정");
            DrawMinCard(m_rcGrpSystem, L"시스템");
            DrawMinCard(m_rcGrpHotkey, L"단축키");
        }
        else if (m_nActiveTab == 3)
        {
            DrawMinCard(m_rcCardShopDown, L"가맹점 다운로드");
        }
    }
}
// ============================================================================
// OnCtlColor
// ============================================================================
HBRUSH CShopSetupDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    /* [UI-STEP] 컨트롤 배경/텍스트 색 커스터마이징(윈도우 기본 칠하기 억제)
     * 1) 에딧/스태틱/버튼 등 컨트롤 종류별로 배경색/텍스트색을 지정한다.
     * 2) 배경을 직접 그리는 경우 투명 배경(SetBkMode TRANSPARENT) 처리한다.
     * 3) 반환하는 브러시는 수명 관리(멤버 브러시 재사용)로 깜빡임과 누수를 방지한다.
     */
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    const int errIds[] = {
        IDC_STATIC_ERR_PORT, IDC_STATIC_ERR_NO_SIGN, IDC_STATIC_ERR_TAX,
        IDC_STATIC_ERR_CARD_DETECT_PROGRAM, IDC_STATIC_ERR_TIMEOUT,
        IDC_STATIC_ERR_SIGNPAD_PORT, IDC_STATIC_ERR_SCANNER_PORT
    };
    for (int i = 0; i < (int)(sizeof(errIds) / sizeof(errIds[0])); ++i)
    {
        if (pWnd && pWnd->GetDlgCtrlID() == errIds[i])
        {
            pDC->SetTextColor(RGB(220, 53, 69));
            pDC->SetBkMode(TRANSPARENT);
            return m_brushBg;
        }
    }
    if (nCtlColor == CTLCOLOR_STATIC && pWnd)
    {
        // SetBkMode(TRANSPARENT): \xc5\xd8\xbd\xba\xc6\xae \xbb\xe7\xc0\xcc \xb9\xe8\xb0\xe6 \xc5\xa9\xb8\xae\xbe\xee
        // \xb9\xdd\xc8\xaf \xba\xea\xb7\xaf\xbd\xcc: \xb4\xd9\xc0\xcc\xbe\xf3\xb7\xce\xb1\xd7 \xb9\xe8\xb0\xe6(249,250,252)\xbf\xcd \xb5\xbf\xc0\xcf \xe2 \xc8\xef\xbb\xf6 \xbb\xe7\xb0\xa2 \xbb\xe7\xb6\xf3\xc1\xfc
        pDC->SetBkMode(TRANSPARENT);
        COLORREF clr = RGB(100, 112, 132);  // label gray
        CString s;
        pWnd->GetWindowText(s);
        if (!s.IsEmpty() && s.GetAt(0) == _T('['))
            clr = RGB(0, 76, 168);  // BLUE_600 for group labels
        pDC->SetTextColor(clr);
        return m_brushBg;  // m_brushWhite -> m_brushBg
    }
    if (nCtlColor == CTLCOLOR_DLG)
        return m_brushBg;
    return hbr;
}
// ============================================================================
// OnDestroy
// ============================================================================
void CShopSetupDlg::OnDestroy()
{
    m_bClosing = TRUE;
    m_bUiInitialized = FALSE;
    if (m_popover.IsVisible()) m_popover.Hide();
    CWnd* pPosInfo = GetDlgItem(IDC_STATIC_CARD_DETECT_POSINFO);
    if (pPosInfo && pPosInfo->GetSafeHwnd())
        pPosInfo->DestroyWindow();
    const int destroyIds[] = {
        IDC_STATIC_SCANNER_PORT_LABEL,
        IDC_STATIC_ERR_PORT, IDC_STATIC_ERR_NO_SIGN, IDC_STATIC_ERR_TAX,
        IDC_STATIC_ERR_CARD_DETECT_PROGRAM, IDC_STATIC_ERR_TIMEOUT,
        IDC_STATIC_ERR_SIGNPAD_PORT, IDC_STATIC_ERR_SCANNER_PORT
    };
    for (int i = 0; i < (int)(sizeof(destroyIds) / sizeof(destroyIds[0])); ++i)
    {
        CWnd* p = GetDlgItem(destroyIds[i]);
        if (p && p->GetSafeHwnd())
            p->DestroyWindow();
    }
    if (m_hFontCardTitle) { ::DeleteObject(m_hFontCardTitle); m_hFontCardTitle = nullptr; }
    if (m_hFontHdrTitle)  { ::DeleteObject(m_hFontHdrTitle);  m_hFontHdrTitle  = nullptr; }
    if (m_hFontHdrSub)    { ::DeleteObject(m_hFontHdrSub);    m_hFontHdrSub    = nullptr; }
    CDialog::OnDestroy();
}
// ============================================================================
// ============================================================================
// Snapshot helpers
// ============================================================================
void CShopSetupDlg::TakeSnapshot()
{
    UpdateData(TRUE);
    m_snap.intPort = m_intPort;
    m_snap.intCardTimeout = m_intCardTimeout;
    m_snap.intNoSignAmount = m_intNoSignAmount;
    m_snap.intTaxPercent = m_intTaxPercent;
    m_snap.intSignPadPort = m_intSignPadPort;
    m_snap.intScannerPort = m_intScannerPort;
    m_snap.strCardDetectParam = m_strCardDetectParam;
    m_snap.cmbVanServer = m_comboVanServer.GetCurSel();
    m_snap.cmbCashReceipt = m_comboCashReceipt.GetCurSel();
    m_snap.cmbInterlock = m_comboInterlock.GetCurSel();
    m_snap.cmbCommType = m_comboCommType.GetCurSel();
    m_snap.cmbSignPadUse = m_comboSignPadUse.GetCurSel();
    m_snap.cmbSignPadSpeed = m_comboSignPadSpeed.GetCurSel();
    m_snap.cmbAlarmPos = m_comboAlarmPos.GetCurSel();
    m_snap.cmbAlarmSize = m_comboAlarmSize.GetCurSel();
    m_snap.cmbCancelKey = m_comboCancelKey.GetCurSel();
    m_snap.cmbMSRKey = m_comboMSRKey.GetCurSel();
    m_snap.tglCardDetect = m_chkCardDetect.IsToggled();
    m_snap.tglMultiVoice = m_chkMultiVoice.IsToggled();
    m_snap.tglScannerUse = m_chkScannerUse.IsToggled();
    m_snap.tglAlarmGraph = m_chkAlarmGraph.IsToggled();
    m_snap.tglAlarmDual = m_chkAlarmDual.IsToggled();
    m_snap.tglAutoReboot = m_chkAutoReboot.IsToggled();
}
BOOL CShopSetupDlg::HasChanges() const
{
    // Read current edit values
    CString s;
    m_editPort.GetWindowText(s);           if (_ttoi(s) != m_snap.intPort)          return TRUE;
    m_editCardTimeout.GetWindowText(s);    if (_ttoi(s) != m_snap.intCardTimeout)   return TRUE;
    m_editNoSignAmount.GetWindowText(s);   if (ParseAmountText(s) != m_snap.intNoSignAmount)  return TRUE;
    m_editTaxPercent.GetWindowText(s);     if (_ttoi(s) != m_snap.intTaxPercent)    return TRUE;
    m_editSignPadPort.GetWindowText(s);    if (_ttoi(s) != m_snap.intSignPadPort)   return TRUE;
    m_editScannerPort.GetWindowText(s);    if (_ttoi(s) != m_snap.intScannerPort)   return TRUE;
    m_editCardDetectParam.GetWindowText(s);if (s != m_snap.strCardDetectParam)      return TRUE;
    if (m_comboVanServer.GetCurSel() != m_snap.cmbVanServer)    return TRUE;
    if (m_comboCashReceipt.GetCurSel() != m_snap.cmbCashReceipt)  return TRUE;
    if (m_comboInterlock.GetCurSel() != m_snap.cmbInterlock)    return TRUE;
    if (m_comboCommType.GetCurSel() != m_snap.cmbCommType)     return TRUE;
    if (m_comboSignPadUse.GetCurSel() != m_snap.cmbSignPadUse)   return TRUE;
    if (m_comboSignPadSpeed.GetCurSel() != m_snap.cmbSignPadSpeed) return TRUE;
    if (m_comboAlarmPos.GetCurSel() != m_snap.cmbAlarmPos)     return TRUE;
    if (m_comboAlarmSize.GetCurSel() != m_snap.cmbAlarmSize)    return TRUE;
    if (m_comboCancelKey.GetCurSel() != m_snap.cmbCancelKey)    return TRUE;
    if (m_comboMSRKey.GetCurSel() != m_snap.cmbMSRKey)       return TRUE;
    if (m_chkCardDetect.IsToggled() != m_snap.tglCardDetect)  return TRUE;
    if (m_chkMultiVoice.IsToggled() != m_snap.tglMultiVoice)  return TRUE;
    if (m_chkScannerUse.IsToggled() != m_snap.tglScannerUse)  return TRUE;
    if (m_chkAlarmGraph.IsToggled() != m_snap.tglAlarmGraph)  return TRUE;
    if (m_chkAlarmDual.IsToggled() != m_snap.tglAlarmDual)   return TRUE;
    if (m_chkAutoReboot.IsToggled() != m_snap.tglAutoReboot)  return TRUE;
    return FALSE;
}
// ============================================================================
// CheckOptionChangesAndNotify -- notify user about specific option changes
// Called from OnOK() after validation, before SaveOptionsToRegistry().
// Shows messages for settings that need user attention.
// ============================================================================
void CShopSetupDlg::CheckOptionChangesAndNotify()
{
    const int nInterlockCount = (int)(sizeof(kInterlock) / sizeof(kInterlock[0]));
    const int nCommTypeCount  = (int)(sizeof(kCommType)  / sizeof(kCommType[0]));

    // Resolve old string values from snapshot indices
    CString oldInterlockVal = (m_snap.cmbInterlock >= 0 && m_snap.cmbInterlock < nInterlockCount)
        ? CString(kInterlock[m_snap.cmbInterlock].value) : CString(_T("NORMAL"));
    CString oldSocketVal = (m_snap.cmbCommType >= 0 && m_snap.cmbCommType < nCommTypeCount)
        ? CString(kCommType[m_snap.cmbCommType].value) : CString(_T(""));

    // Resolve new string values from current UI
    CString newInterlockVal = GetSelectedComboValue(m_comboInterlock, kInterlock, nInterlockCount, _T("NORMAL"));
    CString newSocketVal    = GetSelectedComboValue(m_comboCommType,  kCommType,  nCommTypeCount,  _T(""));

    // WEB mode: value starts with "WEB" (ASCII-safe)
    BOOL bOldWasWeb = (oldSocketVal.GetLength() >= 3 && oldSocketVal.Left(3) == _T("WEB"));
    BOOL bNewIsWeb  = (newSocketVal.GetLength()  >= 3 && newSocketVal.Left(3)  == _T("WEB"));

    // Toggle ON=true -> registry "0", Toggle OFF=false -> registry "1"
    BOOL bCardDetectChanged  = (m_chkCardDetect.IsToggled() != m_snap.tglCardDetect);

    // [2] SOCKET_TYPE_FIELD changed
    if (bNewIsWeb && !bOldWasWeb)
    {
        CModernMessageBox::Warning(
            _T("Communication type changed to WEB mode.\n")
            _T("Please restart the program for the change to take effect."),
            this);

        LaunchExeInSameDir(_T("KFTCOneCAP.exe"));
    }
    else if (!bNewIsWeb && bOldWasWeb)
    {
        CModernMessageBox::Warning(
            _T("Communication type changed to CS mode.\n")
            _T("Please restart the program for the change to take effect."),
            this);

        TerminateExeByName(_T("KFTCOneCAP.exe"));
    }

    // [3] CARD_DETECT_FIELD changed
    if (bCardDetectChanged)
    {
        if (m_chkCardDetect.IsToggled())  // new value = "0" (enabled)
        {
            CModernMessageBox::Info(
                _T("Card detection priority has been enabled.\n")
                _T("Please restart the program for the change to take effect."),
                this);
        }
        else  // new value = "1" (disabled)
        {
            CModernMessageBox::Info(
                _T("Card detection priority has been disabled.\n")
                _T("Please restart the program for the change to take effect."),
                this);
        }
    }

    // [4] INTERLOCK_FIELD was "AOP" and changed to non-AOP
    if (oldInterlockVal == _T("AOP") && newInterlockVal != _T("AOP"))
    {
        CModernMessageBox::Warning(
            _T("AOP reader interlock has been disabled.\n")
            _T("Please check the port settings in reader configuration after restart."),
            this);
    }
}

// ============================================================================
// ValidateComboInputs -- combo-box specific validation rules
// Called from OnOK after ValidateAllInputs passes.
// Returns FALSE if any rule fails (also shows MessageBox and sets focus).
// ============================================================================
BOOL CShopSetupDlg::ValidateComboInputs()
{
    // Rule 1: AOP interlock requires PORT_ALWAYSOPEN enabled (not "0")
    {
        CString interlockVal = GetSelectedComboValue(m_comboInterlock, kInterlock,
            (int)(sizeof(kInterlock)/sizeof(kInterlock[0])), _T("NORMAL"));
        if (interlockVal == _T("AOP"))
        {
            CString portOpen = AfxGetApp()->GetProfileString(
                _T("SERIALPORT"), _T("PORT_ALWAYSOPEN"), _T("1"));
            if (portOpen == _T("0"))
            {
                CModernMessageBox::Warning(_T("AOP reader requires port-always-open to be enabled.\nPlease configure the reader port settings first."), this);
                if (m_nActiveTab != 1) { m_tabCtrl.SetCurSel(1); ShowTab(1); }
                m_comboInterlock.SetFocus();
                return FALSE;
            }
        }
    }
    // Rule 2: VAN server changed but no merchant download has been performed
    if (m_comboVanServer.GetCurSel() != m_snap.cmbVanServer && !m_bMerchantDownloaded)
    {
        CModernMessageBox::Warning(_T("VAN server has been changed. Please perform a merchant download before saving."), this);
        if (m_nActiveTab != 3) { m_tabCtrl.SetCurSel(3); ShowTab(3); }
        m_comboVanServer.SetFocus();
        return FALSE;
    }
    // Rule 3: Cancel hotkey and MSR hotkey must not be the same non-default key
    {
        CString cancelKey = GetSelectedComboValue(m_comboCancelKey, kHotkeys,
            (int)(sizeof(kHotkeys)/sizeof(kHotkeys[0])), _T("NORMAL"));
        CString msrKey    = GetSelectedComboValue(m_comboMSRKey,    kHotkeys,
            (int)(sizeof(kHotkeys)/sizeof(kHotkeys[0])), _T("NORMAL"));
        if (cancelKey != _T("NORMAL") && msrKey != _T("NORMAL") && cancelKey == msrKey)
        {
            CModernMessageBox::Warning(_T("Cancel hotkey and MSR hotkey cannot be set to the same key."), this);
            if (m_nActiveTab != 2) { m_tabCtrl.SetCurSel(2); ShowTab(2); }
            m_comboCancelKey.SetFocus();
            return FALSE;
        }
    }
    return TRUE;
}
// OnOK / OnCancel
// ============================================================================
// --------------------------------------------------------------
// 확인(OK)
//  - SaveOptionsToRegistry() 호출 후 다이얼로그 종료
// --------------------------------------------------------------
void CShopSetupDlg::OnOK()
{
    if (m_popover.GetSafeHwnd()) m_popover.Hide();
    int nFirstInvalidCtrlId = 0;
    if (!ValidateAllInputs(FALSE, &nFirstInvalidCtrlId))
    {
        CModernMessageBox::Warning(_T("입력값을 확인해주세요."), this);
        const int nErrorTab = GetTabIndexForControl(nFirstInvalidCtrlId);
        if (nErrorTab >= 0 && nErrorTab <= 2 && nErrorTab != m_nActiveTab)
        {
            m_tabCtrl.SetCurSel(nErrorTab);
            ShowTab(nErrorTab);
        }
        ValidateAllInputs(TRUE, &nFirstInvalidCtrlId);
        RefreshValidationVisibilityByTab();
        CWnd* pFirst = GetDlgItem(nFirstInvalidCtrlId);
        if (pFirst && pFirst->GetSafeHwnd())
        {
            pFirst->SetFocus();
            if (nFirstInvalidCtrlId == IDC_EDIT_PORT ||
                nFirstInvalidCtrlId == IDC_EDIT_NO_SIGN_AMOUNT ||
                nFirstInvalidCtrlId == IDC_EDIT_TAX_PERCENT ||
                nFirstInvalidCtrlId == IDC_EDIT_CARD_TIMEOUT ||
                nFirstInvalidCtrlId == IDC_EDIT_SIGN_PAD_PORT ||
                nFirstInvalidCtrlId == IDC_EDIT_SCANNER_PORT ||
                nFirstInvalidCtrlId == IDC_EDIT_CARD_DETECT_PARAM)
            {
                ((CEdit*)pFirst)->SetSel(0, -1);
            }
        }
        return;
    }
    if (!ValidateComboInputs())
        return;
    CheckOptionChangesAndNotify();
    SaveOptionsToRegistry();
    CDialog::OnOK();
}
void CShopSetupDlg::OnCancel()
{
    if (m_popover.GetSafeHwnd()) m_popover.Hide();
    if (HasChanges())
    {
        if (CModernMessageBox::Question(_T("변경된 내용이 있습니다.\n저장하지 않고 종료하시겠습니까?\n종료하시겠습니까?"), this) != IDYES)
            return;
    }
    CDialog::OnCancel();
}
// ============================================================================
// DrawInputBorders (하위 호환 stub)
// ============================================================================
void CShopSetupDlg::DrawInputBorders() {
    /* [UI-STEP] 입력 컨트롤 보더 일괄 그리기(에딧/콤보 통일감)
     * 1) 현재 탭에서 관리하는 입력 컨트롤 목록을 순회한다.
     * 2) 각 컨트롤의 화면 좌표(GetWindowRect → ScreenToClient)를 구한다.
     * 3) DrawOneInputBorder()로 라운드 보더/포커스 링/hover 컬러를 적용한다.
     */
}
// ============================================================================
// ShowInfoPopover - unified helper to toggle any info popover
// ============================================================================
void CShopSetupDlg::ShowInfoPopover(CInfoIconButton& btn, LPCTSTR szTitle, LPCTSTR szBody)
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc;
    btn.GetWindowRect(&rc);
    m_popover.ShowAt(rc, szTitle, szBody, this);
}
// ============================================================================
// OnInfoButtonClicked - unified handler for all 18 info popover buttons
//   IDC range: IDC_BTN_VAN_SERVER_INFO (60100) .. IDC_BTN_SIGN_PAD_PORT_INFO (60117)
// ============================================================================
void CShopSetupDlg::OnInfoButtonClicked(UINT nID)
{
    struct InfoEntry { CInfoIconButton* pBtn; LPCTSTR title; LPCTSTR body; };
    static const InfoEntry kTable[] = {
        { &m_btnVanInfo, _T("금융결제원 서버"), _T("금융결제원 서버 선택\n· 실제 거래 서버 : 운영 환경 (기본값)\n· 테스트 서버 : 승인 테스트용\n· 테스트 서버(내부용) : 개발/검증용") },
        { &m_btnPortInfo, _T("포트번호"), _T("금융결제원 서버 접속 포트번호\n· 기본값 : 8002") },
        { &m_btnCommTypeInfo, _T("통신방식"), _T("포스 프로그램 통신 방식 선택\n· CS 방식: 윈도우 포스 프로그램 (기본값)\n· WEB 방식: WEB 포스 프로그램 (EASYPOS 포함)") },
        { &m_btnCashReceiptInfo, _T("현금영수증 거래"), _T("현금영수증 승인시 입력 방식 선택\n· PINPAD/KEYIN : PINPAD/KEYIN 동시 입력 (기본값)\n· MS : MS 카드 입력\n· KEYIN : KEYIN 입력") },
        { &m_btnCardTimeoutInfo, _T("카드입력 Timeout"), _T("카드 입력 대기 시간 (초 단위)\n· 권장값: 100초 / 0 입력 시 자동 100초 설정") },
        { &m_btnInterlockInfo, _T("장치 연동 방식"), _T("카드 리더기 연동 방식 선택\n· IC/MS 리더기: 일반 리더기 (기본값)\n· LockType리더기(TDR): TDR 방식 리더기\n· AutoDriven리더기(TTM): TTM 방식 리더기\n· 단말기(forPOS): 단말기 연동 거래\n· 멀티패드(동반위): 멀티패드 및 신형 리더기 사용 (권장값)\n· AOP 리더기: AOP 리더기(Naver Connect 포함)") },
        { &m_btnSignPadUseInfo, _T("서명패드 사용"), _T("서명패드 사용여부 설정\n· 예 : 서명패드를 사용하는 경우\n· 아니오 : 서명패드를 사용하지 않는 경우\n· 자체서명 : 포스 화면에서 서명 입력") },
        { &m_btnSignPadSpeedInfo, _T("서명패드 속도"), _T("서명패드 통신 속도 선택\n· 115200bps: 멀티패드 사용 시\n· 57600bps: 서명패드 사용 시") },
        { &m_btnAlarmSizeInfo, _T("알림창 크기"), _T("거래 알림창의 표시 크기를 설정합니다.\n· 기본값 : 매우작게 ") },
        { &m_btnMultiVoiceInfo, _T("음성출력"), _T("카드 리딩 시 음성 출력 여부\n· 기본값 : 미사용\n※SPAY-8800Q, DP636X 모델만 가능") },
        { &m_btnCardDetectInfo, _T("카드 감지 우선 거래 사용"), _T("카드 감지 우선 거래 사용 여부 설정\n· 기본값 : 미사용\n입력창에는 POS 프로그램 정보 입력(POS 프로그램 업체 안내 필요)\n※우선 거래가 개발된 POS 프로그램만 사용") },
        { &m_btnScannerUseInfo, _T("스캐너 사용"), _T("스캐너 사용 여부 설정\n· 기본값 : 미사용\n입력창에는 포트번호 입력\n※KFTCOneCAP에서 외부 스캐너를 연동하는 경우 사용 \n※POS 프로그램에서 연동하는 경우 사용 X") },
        { &m_btnAutoRebootInfo, _T("자동 리부팅"), _T("일일 단위 KFTCOneCAP 자동 리부팅 여부\n· 기본값 : 사용") },
        { &m_btnAlarmGraphInfo, _T("알림창 그림"), _T("거래 알림창 이미지 출력 여부\n· 기본값: 사용") },
        { &m_btnAlarmDualInfo, _T("알림창 듀얼"), _T("듀얼 모니터 사용 시 서브 모니터에 알림창 출력\n· 기본값: 미사용") },
        { &m_btnTaxPercentInfo, _T("세금 자동역산 설정"), _T("세금 자동 계산 비율 (%)\n· 기본값: 0 (0=세금 없음, 10=공급가액에서 10% 역산)\n※ POS에서 세금 필드를 채우지 않는 경우에만 적용") },
        { &m_btnSignPadPortInfo, _T("서명패드 포트번호"), _T("서명패드가 연결된 COM 포트번호") }
    };
    for (int i = 0; i < _countof(kTable); i++)
    {
        if (kTable[i].pBtn->GetSafeHwnd() && (UINT)kTable[i].pBtn->GetDlgCtrlID() == nID)
        {
            ShowInfoPopover(*kTable[i].pBtn, kTable[i].title, kTable[i].body);
            return;
        }
    }
}
const CShopSetupDlg::ValidationBinding* CShopSetupDlg::GetValidationBindings(int& outCount)
{
    static const ValidationBinding kBindings[] = {
        { IDC_EDIT_PORT,              IDC_STATIC_ERR_PORT,                VF_PORT,                 0 },
        { IDC_EDIT_NO_SIGN_AMOUNT,    IDC_STATIC_ERR_NO_SIGN,             VF_NO_SIGN_AMOUNT,       0 },
        { IDC_EDIT_TAX_PERCENT,       IDC_STATIC_ERR_TAX,                 VF_TAX_PERCENT,          0 },
        { IDC_EDIT_CARD_DETECT_PARAM, IDC_STATIC_ERR_CARD_DETECT_PROGRAM, VF_CARD_DETECT_PROGRAM,  0 },
        { IDC_EDIT_CARD_TIMEOUT,      IDC_STATIC_ERR_TIMEOUT,             VF_CARD_TIMEOUT,         1 },
        { IDC_EDIT_SIGN_PAD_PORT,     IDC_STATIC_ERR_SIGNPAD_PORT,        VF_SIGNPAD_PORT,         1 },
        { IDC_EDIT_SCANNER_PORT,      IDC_STATIC_ERR_SCANNER_PORT,        VF_SCANNER_PORT,         1 }
    };
    outCount = (int)(sizeof(kBindings) / sizeof(kBindings[0]));
    return kBindings;
}
const CShopSetupDlg::ValidationBinding* CShopSetupDlg::FindValidationBinding(int nCtrlId) const
{
    int count = 0;
    const ValidationBinding* bindings = GetValidationBindings(count);
    for (int i = 0; i < count; ++i)
    {
        if (bindings[i].ctrlId == nCtrlId)
            return &bindings[i];
    }
    return NULL;
}
const CShopSetupDlg::ValidationBinding* CShopSetupDlg::FindValidationBindingByErrId(int nStaticId) const
{
    int count = 0;
    const ValidationBinding* bindings = GetValidationBindings(count);
    for (int i = 0; i < count; ++i)
    {
        if (bindings[i].errId == nStaticId)
            return &bindings[i];
    }
    return NULL;
}
BOOL CShopSetupDlg::IsDigitsOnly(const CString& text)
{
    if (text.IsEmpty())
        return FALSE;
    for (int i = 0; i < text.GetLength(); ++i)
    {
        if (_istspace(text[i]))
            continue;
        if (!_istdigit(text[i]))
            return FALSE;
    }
    return TRUE;
}
BOOL CShopSetupDlg::IsPositiveNumberText(const CString& text)
{
    if (!IsDigitsOnly(text))
        return FALSE;
    return (_ttoi(text) > 0);
}
void CShopSetupDlg::SetValidationText(int nStaticId, const CString& text)
{
    if (m_bClosing || !GetSafeHwnd())
        return;
    CWnd* p = GetDlgItem(nStaticId);
    if (!p || !p->GetSafeHwnd())
        return;
    const ValidationBinding* binding = FindValidationBindingByErrId(nStaticId);
    const BOOL bShouldShow = (!text.IsEmpty() && binding && binding->tabIndex == m_nActiveTab);
    const BOOL bVisibleNow = (p->IsWindowVisible() != FALSE);
    CString oldText;
    p->GetWindowText(oldText);
    if (oldText == text && bVisibleNow == bShouldShow)
        return;
    p->SetRedraw(FALSE);
    if (oldText != text)
        p->SetWindowText(text);
    if (bVisibleNow != bShouldShow)
        p->ShowWindow(bShouldShow ? SW_SHOW : SW_HIDE);
    p->SetRedraw(TRUE);
    p->Invalidate(FALSE);
}
// --------------------------------------------------------------
// 현재 활성 탭의 오류문구만 보이도록 정리
//  - 다른 탭에 속한 오류 Static은 숨긴다.
//  - 확인 버튼 검증 후 탭 이동/탭 클릭 전환 직후에 호출된다.
// --------------------------------------------------------------
void CShopSetupDlg::RefreshValidationVisibilityByTab()
{
    if (m_bClosing || !GetSafeHwnd())
        return;
    int count = 0;
    const ValidationBinding* bindings = GetValidationBindings(count);
    for (int i = 0; i < count; ++i)
    {
        CWnd* pErr = GetDlgItem(bindings[i].errId);
        if (!pErr || !pErr->GetSafeHwnd())
            continue;
        CString text;
        pErr->GetWindowText(text);
        text.Trim();
        const BOOL bShouldShow = (!text.IsEmpty() && bindings[i].tabIndex == m_nActiveTab);
        const BOOL bVisibleNow = (pErr->IsWindowVisible() != FALSE);
        if (bVisibleNow != bShouldShow)
            pErr->ShowWindow(bShouldShow ? SW_SHOW : SW_HIDE);
    }
}
CSkinnedEdit* CShopSetupDlg::GetSkinnedEditByCtrlId(int nCtrlId)
{
    switch (nCtrlId)
    {
    case IDC_EDIT_PORT:              return &m_editPort;
    case IDC_EDIT_NO_SIGN_AMOUNT:    return &m_editNoSignAmount;
    case IDC_EDIT_TAX_PERCENT:       return &m_editTaxPercent;
    case IDC_EDIT_CARD_TIMEOUT:      return &m_editCardTimeout;
    case IDC_EDIT_CARD_DETECT_PARAM: return &m_editCardDetectParam;
    case IDC_EDIT_SIGN_PAD_PORT:     return &m_editSignPadPort;
    case IDC_EDIT_SCANNER_PORT:      return &m_editScannerPort;
    default:                         return NULL;
    }
}
void CShopSetupDlg::SetEditValidationErrorState(int nCtrlId, BOOL bHasError)
{
    if (m_bClosing || !GetSafeHwnd())
        return;
    CSkinnedEdit* pEdit = GetSkinnedEditByCtrlId(nCtrlId);
    if (!pEdit || !pEdit->GetSafeHwnd())
        return;
    pEdit->SetValidationError(bHasError);
}
void CShopSetupDlg::PositionValidationText(int nStaticId, int x, int y, int w, int h, BOOL bShow)
{
    if (m_bClosing || !GetSafeHwnd())
        return;
    CWnd* p = GetDlgItem(nStaticId);
    if (!p || !p->GetSafeHwnd())
        return;
    CRect rcNow;
    p->GetWindowRect(&rcNow);
    ScreenToClient(&rcNow);
    if (rcNow.left != x || rcNow.top != y || rcNow.Width() != w || rcNow.Height() != h)
        p->SetWindowPos(NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    const BOOL bVisibleNow = (p->IsWindowVisible() != FALSE);
    if (bVisibleNow != bShow)
        p->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
}
void CShopSetupDlg::EnsureValidationStatics()
{
    if (!GetSafeHwnd() || m_bClosing)
        return;
    const int ids[] = {
        IDC_STATIC_ERR_PORT, IDC_STATIC_ERR_NO_SIGN, IDC_STATIC_ERR_TAX,
        IDC_STATIC_ERR_CARD_DETECT_PROGRAM, IDC_STATIC_ERR_TIMEOUT,
        IDC_STATIC_ERR_SIGNPAD_PORT, IDC_STATIC_ERR_SCANNER_PORT
    };
    for (int i = 0; i < (int)(sizeof(ids) / sizeof(ids[0])); ++i)
    {
        if (::IsWindow(::GetDlgItem(m_hWnd, ids[i])))
            continue;
        HWND hStatic = ::CreateWindowEx(0, _T("STATIC"), _T(""), WS_CHILD | SS_RIGHT,
            0, 0, 0, 0, m_hWnd, (HMENU)ids[i], AfxGetInstanceHandle(), NULL);
        if (hStatic)
        {
            ::SendMessage(hStatic, WM_SETFONT, (WPARAM)(HFONT)m_fontValidation.GetSafeHandle(), TRUE);
            ::ShowWindow(hStatic, SW_HIDE);
        }
    }
}
BOOL CShopSetupDlg::ValidateSingleField(ValidationField field, CString& outMessage) const
{
    outMessage.Empty();
    auto GetTrimmed = [&](int nCtrlId, CString& out) -> BOOL
        {
            CWnd* p = GetDlgItem(nCtrlId);
            if (!p || !p->GetSafeHwnd())
                return FALSE;
            p->GetWindowText(out);
            out.Trim();
            return TRUE;
        };
    auto IsCtrlEnabled = [&](int nCtrlId) -> BOOL
        {
            CWnd* p = GetDlgItem(nCtrlId);
            return (p && p->GetSafeHwnd() && p->IsWindowEnabled());
        };
    CString s;
    switch (field)
    {
    case VF_PORT:
        if (!GetTrimmed(IDC_EDIT_PORT, s) || !IsDigitsOnly(s))
            outMessage = _T("포트번호 입력");
        break;
    case VF_NO_SIGN_AMOUNT:
        if (GetTrimmed(IDC_EDIT_NO_SIGN_AMOUNT, s)) { CString _t = s; _t.Remove(_T(',')); s = _t; }
        if (!IsDigitsOnly(s))
            outMessage = _T("금액 입력");
        break;
    case VF_TAX_PERCENT:
        if (!GetTrimmed(IDC_EDIT_TAX_PERCENT, s) || !IsDigitsOnly(s) || _ttoi(s) < 0 || _ttoi(s) > 100)
            outMessage = _T("0~100 입력");
        break;
    case VF_CARD_DETECT_PROGRAM:
        if (m_chkCardDetect.IsToggled())
        {
            if (!IsCtrlEnabled(IDC_EDIT_CARD_DETECT_PARAM))
                break;
            if (!GetTrimmed(IDC_EDIT_CARD_DETECT_PARAM, s) || s.IsEmpty())
                outMessage = _T("필수 입력");
        }
        break;
    case VF_CARD_TIMEOUT:
        if (!GetTrimmed(IDC_EDIT_CARD_TIMEOUT, s) || !IsDigitsOnly(s) || (_ttoi(s) != 0 && _ttoi(s) < 30))
            outMessage = _T("30초 이상 입력");
        break;
    case VF_SIGNPAD_PORT:
        if (m_comboSignPadUse.GetCurSel() == 0)
        {
            if (!IsCtrlEnabled(IDC_EDIT_SIGN_PAD_PORT))
                break;
            if (!GetTrimmed(IDC_EDIT_SIGN_PAD_PORT, s) || !IsPositiveNumberText(s))
                outMessage = _T("포트번호 입력");
        }
        break;
    case VF_SCANNER_PORT:
        if (m_chkScannerUse.IsToggled())
        {
            if (!IsCtrlEnabled(IDC_EDIT_SCANNER_PORT))
                break;
            if (!GetTrimmed(IDC_EDIT_SCANNER_PORT, s) || !IsPositiveNumberText(s))
                outMessage = _T("포트번호 입력");
        }
        break;
    default:
        break;
    }
    return outMessage.IsEmpty();
}
int CShopSetupDlg::GetTabIndexForControl(int nCtrlId) const
{
    const ValidationBinding* binding = FindValidationBinding(nCtrlId);
    return binding ? binding->tabIndex : -1;
}
// --------------------------------------------------------------
// Edit 1개 실시간 검증 + UI 갱신
//  - EN_CHANGE에서 호출된다.
//  - 현재 Edit 1개만 검사해서 깜빡임 없이 오류문구/빨간테두리만 갱신한다.
//  - 비활성화 상태이거나 생성 전/종료 중이면 아무 것도 하지 않는다.
// --------------------------------------------------------------
void CShopSetupDlg::ValidateControlAndUpdateUI(int nCtrlId)
{
    if (!m_bUiInitialized || m_bClosing || !GetSafeHwnd())
        return;
    const ValidationBinding* binding = FindValidationBinding(nCtrlId);
    if (!binding)
        return;
    CString err;
    ValidateSingleField(binding->field, err);
    SetValidationText(binding->errId, err);
    SetEditValidationErrorState(binding->ctrlId, !err.IsEmpty());
}
// --------------------------------------------------------------
// 전체 입력값 검증
//  - 확인 버튼에서 사용한다.
//  - bUpdateUI=FALSE : 알림창을 띄우기 전, 조용히 오류 여부만 판단
//  - bUpdateUI=TRUE  : 오류문구/오류테두리까지 화면에 반영
//  - 첫 번째 오류 Edit의 CtrlId를 pFirstInvalidCtrlId로 반환해 탭 이동/포커스에 사용한다.
// --------------------------------------------------------------
BOOL CShopSetupDlg::ValidateAllInputs(BOOL bUpdateUI, int* pFirstInvalidCtrlId)
{
    if (m_bClosing || !GetSafeHwnd())
        return TRUE;
    if (pFirstInvalidCtrlId)
        *pFirstInvalidCtrlId = 0;
    BOOL bAllValid = TRUE;
    int count = 0;
    const ValidationBinding* bindings = GetValidationBindings(count);
    for (int i = 0; i < count; ++i)
    {
        CString err;
        ValidateSingleField(bindings[i].field, err);
        if (bUpdateUI)
        {
            SetValidationText(bindings[i].errId, err);
            SetEditValidationErrorState(bindings[i].ctrlId, !err.IsEmpty());
        }
        if (!err.IsEmpty())
        {
            bAllValid = FALSE;
            if (pFirstInvalidCtrlId && *pFirstInvalidCtrlId == 0)
                *pFirstInvalidCtrlId = bindings[i].ctrlId;
        }
    }
    return bAllValid;
}
void CShopSetupDlg::OnEnChangeValidateInput()
{
    if (!m_bUiInitialized || m_bClosing || !GetSafeHwnd())
        return;
    CWnd* pFocus = GetFocus();
    if (!pFocus || !pFocus->GetSafeHwnd())
        return;
    const int nCtrlId = pFocus->GetDlgCtrlID();
    if (!FindValidationBinding(nCtrlId))
        return;
    if (nCtrlId == IDC_EDIT_NO_SIGN_AMOUNT)
    {
        static BOOL s_bFormatting = FALSE;
        if (!s_bFormatting)
        {
            s_bFormatting = TRUE;
            CString raw;
            m_editNoSignAmount.GetWindowText(raw);
            int nStart = 0, nEnd = 0;
            m_editNoSignAmount.GetSel(nStart, nEnd);
            int digitsBefore = 0;
            for (int i = 0; i < (int)nStart && i < raw.GetLength(); ++i)
                if (_istdigit(raw[i])) digitsBefore++;
            CString digits;
            for (int i = 0; i < raw.GetLength(); ++i)
                if (_istdigit(raw[i])) digits += raw[i];
            CString formatted = FormatAmountWithCommas(_ttoi(digits));
            if (formatted != raw)
            {
                m_editNoSignAmount.SetWindowText(formatted);
                int newPos = 0, dc = 0;
                for (int i = 0; i < formatted.GetLength() && dc < digitsBefore; ++i)
                {
                    if (_istdigit(formatted[i])) dc++;
                    newPos = i + 1;
                }
                m_editNoSignAmount.SetSel(newPos, newPos);
            }
            s_bFormatting = FALSE;
        }
    }
    ValidateControlAndUpdateUI(nCtrlId);
}
void CShopSetupDlg::DrawInputBorders(CDC* /*pDC*/) {}
void CShopSetupDlg::DrawOneInputBorder(int /*ctrlId*/) {
    /* [UI-STEP] 단일 입력 보더 렌더링(포커스/hover/disabled 상태 반영)
     * 1) 컨트롤 Enabled 여부에 따라 보더/배경/텍스트 톤을 선택한다.
     * 2) 포커스가 있으면 포커스 링(강조 보더)을 그린다.
     * 3) hover 상태면 hover 보더 색을 적용한다.
     * 4) 라운드 사각형 Path로 외곽선을 그려 모서리 일관성을 유지한다.
     */
}
void CShopSetupDlg::DrawOneInputBorder(CDC* /*pDC*/, int /*ctrlId*/) {}
// ============================================================================
// OnDrawItem / OnMeasureItem
// owner-draw 컨트롤(CModernButton, CModernCheckBox 등)의 그리기 메시지를
// 컨트롤 자신에게 반사(Reflect)합니다.
// ============================================================================
void CShopSetupDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    /* [UI-STEP] Owner-draw 컨트롤 그리기(버튼/콤보/리스트 등 커스텀 렌더)
     * 1) lpDIS로 넘어오는 controlID를 확인해 어떤 컨트롤을 그릴지 분기한다.
     * 2) 상태(선택/포커스/비활성/pressed/hover)를 읽어 색/보더/텍스트를 결정한다.
     * 3) 커스텀 그리기 후 기본 그리기가 덮지 않도록 필요한 경우 기본 처리 호출을 막는다.
     */
     // owner-draw 컨트롤이 반사된 WM_DRAWITEM을 직접 처리하므로
     // 기본 구현(CDialog::OnDrawItem)을 호출합니다.
    CDialog::OnDrawItem(nIDCtl, lpDIS);
}
void CShopSetupDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
    /* [UI-STEP] Owner-draw 아이템 높이 계산(콤보 드랍리스트 등)
     * 1) 드랍리스트 항목 높이를 DPI/폰트에 맞게 산출한다.
     * 2) 너무 작으면 텍스트가 잘리므로 최소 높이를 보장한다.
     */
    CDialog::OnMeasureItem(nIDCtl, lpMIS);
}
// ============================================================================
// DrawSectionIcon (하위 호환 stub - 탭 UI에서는 미사용)
// ============================================================================
void CShopSetupDlg::DrawSectionIcon(CDC* /*pDC*/, const CRect& /*rcIcon*/,
    SECTION_ICON_TYPE /*iconType*/) {
}
// ============================================================================
// Timer (입력 hover 추적 - 필요시 확장)
// ============================================================================
void CShopSetupDlg::OnTimer(UINT_PTR nIDEvent)
{
    CDialog::OnTimer(nIDEvent);
}
void CShopSetupDlg::UpdateInputHoverByCursor() {}
// ============================================================================
// v10.1 - Toggle dependent edit enable/disable
// ============================================================================
// --------------------------------------------------------------
// 토글/콤보 상태에 따른 종속 Edit Enable/Disable 처리
//  - 우선 거래 프로그램 / 스캐너 포트번호 / 서명패드 포트번호가 대상
//  - 비활성화되는 순간 오류문구와 오류테두리를 즉시 제거한다.
//  - 활성화되는 순간에는 현재 값 기준으로 즉시 다시 검증한다.
// --------------------------------------------------------------
void CShopSetupDlg::UpdateToggleDependentEdits(BOOL bForceRedraw /*= TRUE*/)
{
    if (!GetSafeHwnd() || m_bClosing)
        return;
    // Card detect (priority transaction)
    if (m_editCardDetectParam.GetSafeHwnd() && m_chkCardDetect.GetSafeHwnd())
    {
        const BOOL bEnable = m_chkCardDetect.IsToggled();
        const BOOL bPrevEnable = m_editCardDetectParam.IsWindowEnabled();
        if (bPrevEnable != bEnable)
            m_editCardDetectParam.EnableWindow(bEnable);
        if (!bEnable && ::GetFocus() == m_editCardDetectParam.GetSafeHwnd())
            m_tabCtrl.SetFocus();
        if (!bEnable)
        {
            SetValidationText(IDC_STATIC_ERR_CARD_DETECT_PROGRAM, _T(""));
            SetEditValidationErrorState(IDC_EDIT_CARD_DETECT_PARAM, FALSE);
        }
        else if (m_bUiInitialized)
        {
            ValidateControlAndUpdateUI(IDC_EDIT_CARD_DETECT_PARAM);
        }
        if (bForceRedraw && bPrevEnable != bEnable)
            m_editCardDetectParam.Invalidate(FALSE);
    }
    // Scanner use
    if (m_editScannerPort.GetSafeHwnd() && m_chkScannerUse.GetSafeHwnd())
    {
        const BOOL bEnable = m_chkScannerUse.IsToggled();
        const BOOL bPrevEnable = m_editScannerPort.IsWindowEnabled();
        if (bPrevEnable != bEnable)
            m_editScannerPort.EnableWindow(bEnable);
        if (!bEnable && ::GetFocus() == m_editScannerPort.GetSafeHwnd())
            m_tabCtrl.SetFocus();
        if (!bEnable)
        {
            SetValidationText(IDC_STATIC_ERR_SCANNER_PORT, _T(""));
            SetEditValidationErrorState(IDC_EDIT_SCANNER_PORT, FALSE);
        }
        else if (m_bUiInitialized)
        {
            ValidateControlAndUpdateUI(IDC_EDIT_SCANNER_PORT);
        }
        if (bForceRedraw && bPrevEnable != bEnable)
            m_editScannerPort.Invalidate(FALSE);
    }
    // Sign pad port + speed (YES=index 0 only)
    if (m_comboSignPadUse.GetSafeHwnd() && m_editSignPadPort.GetSafeHwnd() && m_comboSignPadSpeed.GetSafeHwnd())
    {
        const BOOL bEnable = (m_comboSignPadUse.GetCurSel() == 0);
        const BOOL bPrevEditEnable = m_editSignPadPort.IsWindowEnabled();
        const BOOL bPrevComboEnable = m_comboSignPadSpeed.IsWindowEnabled();
        if (bPrevEditEnable != bEnable)
            m_editSignPadPort.EnableWindow(bEnable);
        if (bPrevComboEnable != bEnable)
            m_comboSignPadSpeed.EnableWindow(bEnable);
        if (!bEnable && ::GetFocus() == m_editSignPadPort.GetSafeHwnd())
            m_tabCtrl.SetFocus();
        if (!bEnable)
        {
            SetValidationText(IDC_STATIC_ERR_SIGNPAD_PORT, _T(""));
            SetEditValidationErrorState(IDC_EDIT_SIGN_PAD_PORT, FALSE);
        }
        else if (m_bUiInitialized)
        {
            ValidateControlAndUpdateUI(IDC_EDIT_SIGN_PAD_PORT);
        }
        if (bForceRedraw)
        {
            if (bPrevEditEnable != bEnable)
                m_editSignPadPort.Invalidate(FALSE);
            if (bPrevComboEnable != bEnable)
                m_comboSignPadSpeed.Invalidate(FALSE);
        }
    }
}
void CShopSetupDlg::OnCbnSelchangeSignPadUse()
{
    if (!m_bUiInitialized || m_bClosing || !GetSafeHwnd())
        return;
    UpdateToggleDependentEdits(TRUE);
}
void CShopSetupDlg::OnBnClickedCardDetectToggle()
{
    if (!m_bUiInitialized || m_bClosing || !GetSafeHwnd())
        return;
    UpdateToggleDependentEdits(TRUE);
    ValidateControlAndUpdateUI(IDC_EDIT_CARD_DETECT_PARAM);
}
void CShopSetupDlg::OnBnClickedScannerUseToggle()
{
    if (!m_bUiInitialized || m_bClosing || !GetSafeHwnd())
        return;
    UpdateToggleDependentEdits(TRUE);
    ValidateControlAndUpdateUI(IDC_EDIT_SCANNER_PORT);
}
