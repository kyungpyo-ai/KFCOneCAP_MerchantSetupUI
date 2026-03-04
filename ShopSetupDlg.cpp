// ShopSetupDlg.cpp - 탭 UI 버전 (v1.8)
// 4개 탭(서버 연결 / 장치 정보 / 시스템 코드 / 가맹점 다운로드)

#include "stdafx.h"
#include "Resource.h"
#include "ShopSetupDlg.h"
#include "ShopDownDlg.h"
#include "ModernUI.h"
#include "RegistryUtil.h"

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// ============================================================================
// Registry spec (Word 기준)
// - 저장: AfxGetApp()->WriteProfileString(section, field, value) 그대로 사용
// - 불러오기: GetRegisterData(section, field, outValue) 로만 접근
// ============================================================================

namespace
{
    // Sections
    static LPCTSTR SEC_TCP        = _T("TCP");
    static LPCTSTR SEC_SERIALPORT = _T("SERIALPORT");

    // TCP
    static LPCTSTR VAN_SERVER_IP_FIELD   = _T("VAN_SERVER_IP");
    static LPCTSTR VAN_SERVER_PORT_FIELD = _T("VAN_SERVER_PORT");
    static LPCTSTR TAX_SETTING_FIELD     = _T("TAX_SETTING");   // 세금 자동 역산 (IDC_EDIT_TAX_PERCENT)

    // SERIALPORT
    static LPCTSTR TIMEOUT_FIELD         = _T("TIMEOUT");
    static LPCTSTR NOSIGN_AMT_FIELD      = _T("NOSIGN_AMT");
    static LPCTSTR CASH_FIRST_FIELD      = _T("CASH_FIRST");
    static LPCTSTR INTERLOCK_FIELD       = _T("INTERLOCK");
    static LPCTSTR SOCKET_TYPE_FIELD     = _T("SOCKET_TYPE");
    static LPCTSTR SIGNPAD_USE_FIELD     = _T("SIGNPAD_USE");
    static LPCTSTR SIGNPAD_FIELD         = _T("SIGNPAD");
    static LPCTSTR SIGNPAD_SPEED_FIELD   = _T("SIGNPAD_SPEED");
    static LPCTSTR NOTIFY_POS_FIELD      = _T("NOTIFY_POS");
    static LPCTSTR NOTIFY_SIZE_FIELD     = _T("NOTIFY_SIZE");
    static LPCTSTR CANCEL_HOTKEY_FIELD   = _T("CANCEL_HOTKEY");
    static LPCTSTR MSR_HOTKEY_FIELD      = _T("MSR_HOTKEY");
    static LPCTSTR MULTIPAD_SOUND_FIELD  = _T("MULTIPAD_SOUND");
    static LPCTSTR BARCODE_USE_FIELD     = _T("BARCODE_USE");
    static LPCTSTR BARCODE_PORT_FIELD    = _T("BARCODE_PORT");
    static LPCTSTR CARD_DETECT_FIELD     = _T("CARD_DETECT");
    static LPCTSTR DETECT_PROGRAM_FIELD  = _T("DETECT_PROGRAM");
    static LPCTSTR AUTO_RESTART_FIELD    = _T("AUTO_RESTART");
    static LPCTSTR AUTO_REBOOT_FIELD     = _T("AUTO_REBOOT");
    static LPCTSTR NOTIFY_IMG_FIELD      = _T("NOTIFY_IMG");
    static LPCTSTR NOTIFY_DUAL_FIELD     = _T("NOTIFY_DUAL_MONITOR");

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
        { _T("LockType 리더기"),       _T("LOCKTYPE(TDR)") },
        { _T("AutoDriven 리더기"),     _T("LOCKTYPE(TTM)") },
        { _T("단말기(forPOS)"),        _T("FORPOS") },
        { _T("멀티패드(복지단)"),      _T("DP636-MND") },
        { _T("멀티패드(동반위)"),      _T("TRANSINFO") },
        { _T("멀티패드(씨큐프라임)"),  _T("CQPRIME") },
        { _T("멀티패드(키오스크)"),    _T("KIOSK") },
        { _T("AOP 리더기"),            _T("AOP") },
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
} // namespace
// ============================================================================
// [TUNE] 헤더 / 탭 / 컨텐츠 레이아웃 튜닝 파라미터
// ============================================================================

// ── 헤더 영역 ────────────────────────────────────────────────────────────────
// [TUNE] kHdrBadgeY    : 배지 아이콘 상단 Y (다이얼로그 좌상단 기준)
static const int kHdrBadgeY      = 28;   // [TUNE] 배지 상단 위치 (기본 20)
// [TUNE] kHdrBadgeSz   : 배지 크기(정사각형)
static const int kHdrBadgeSz     = 38;   // [TUNE] 배지 크기 px (기본 38)
// [TUNE] kHdrBadgeX    : 배지 좌측 여백
static const int kHdrBadgeX      = 26;   // [TUNE] 배지 좌측 여백 (기본 26)
// [TUNE] kHdrTitleGap  : 배지-텍스트 간격
static const int kHdrTitleGap    = 13;   // [TUNE] 배지→텍스트 간격 (기본 13)
// [TUNE] kHdrDividerY  : 헤더 하단 구분선 Y 위치
static const int kHdrDividerY    = 84;   // [TUNE] 헤더 구분선 Y (기본 76)

// ── 탭 컨트롤 ────────────────────────────────────────────────────────────────
// [TUNE] kTabBarTop : 탭 바 상단 Y (헤더 구분선 아래)
static const int kTabBarTop     = kHdrDividerY + 6;  // [TUNE] 탭 바 시작 Y
static const int kTabBarH       = 34;                // [TUNE] 탭 바 높이
static const int kTabPadTop     = 12;                // [TUNE] 탭 내용 상단 여백
static const int kTabPadLeft    = 40;                // [TUNE] 탭 내용 좌측 여백

// [DEPRECATED] kHeaderShiftY - 하위 호환용 (새 코드는 kHdrDividerY 사용)
static const int kHeaderShiftY = kHdrDividerY - 88; // 자동 계산

// 컨텐츠 시작 Y = kTabBarTop + kTabBarH + kTabPadTop
static const int kContentStartY = kTabBarTop + kTabBarH + kTabPadTop;

// ── 그룹/카드 공통 ───────────────────────────────────────────────────────────
static const int kGroupTitleH        = 20;
static const int kGroupGapBelowTitle = 1;
static const int kGapToNextGroup     = 6;

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
    ON_BN_CLICKED(IDC_BTN_VAN_SERVER_INFO,     OnBnClickedVanServerInfo)
    ON_BN_CLICKED(IDC_BTN_PORT_INFO,           OnBnClickedPortInfo)
    ON_BN_CLICKED(IDC_BTN_TAX_PERCENT_INFO,   OnBnClickedTaxPercentInfo)
    ON_BN_CLICKED(IDC_BTN_COMM_TYPE_INFO,      OnBnClickedCommTypeInfo)
    ON_BN_CLICKED(IDC_BTN_CASH_RECEIPT_INFO,   OnBnClickedCashReceiptInfo)
    ON_BN_CLICKED(IDC_BTN_CARD_TIMEOUT_INFO,   OnBnClickedCardTimeoutInfo)
    ON_BN_CLICKED(IDC_BTN_INTERLOCK_INFO,      OnBnClickedInterlockInfo)
    ON_BN_CLICKED(IDC_BTN_MULTI_VOICE_INFO,    OnBnClickedMultiVoiceInfo)
    ON_BN_CLICKED(IDC_BTN_CARD_DETECT_INFO,   OnBnClickedCardDetectInfo)
    ON_BN_CLICKED(IDC_BTN_SCANNER_USE_INFO,   OnBnClickedScannerUseInfo)
    ON_BN_CLICKED(IDC_BTN_AUTO_RESET_INFO,    OnBnClickedAutoResetInfo)
    ON_BN_CLICKED(IDC_BTN_AUTO_REBOOT_INFO,   OnBnClickedAutoRebootInfo)
    ON_BN_CLICKED(IDC_BTN_ALARM_GRAPH_INFO,   OnBnClickedAlarmGraphInfo)
    ON_BN_CLICKED(IDC_BTN_ALARM_DUAL_INFO,    OnBnClickedAlarmDualInfo)
    ON_BN_CLICKED(IDC_BTN_SIGN_PAD_USE_INFO,   OnBnClickedSignPadUseInfo)
    ON_BN_CLICKED(IDC_BTN_SIGN_PAD_PORT_INFO,  OnBnClickedSignPadPortInfo)
    ON_BN_CLICKED(IDC_BTN_SIGN_PAD_SPEED_INFO, OnBnClickedSignPadSpeedInfo)
    ON_BN_CLICKED(IDC_BTN_ALARM_SIZE_INFO,     OnBnClickedAlarmSizeInfo)
    ON_BN_CLICKED(IDC_CHECK_CARD_DETECT,        OnBnClickedCardDetectToggle)
    ON_BN_CLICKED(IDC_CHECK_SCANNER_USE,        OnBnClickedScannerUseToggle)
END_MESSAGE_MAP()

// ============================================================================
// OnNcActivate  [FIX v2.1] - DefDlgProc xxxSaveDlgFocus O(N^2) SendMessage 차단
// ============================================================================
// [문제] DefDlgProc(WM_NCACTIVATE) 내부의 xxxSaveDlgFocus 가 모든 자식/손자
//        버튼에 BM_SETSTYLE 을 SendMessage 한다.
//        CShopDownDlg 에 CModernButton 이 50개(다운로드 25 + 삭제 25) 있으므로
//        각 BM_SETSTYLE -> DefWindowProc -> DM_SETDEFID -> DefDlgProc 재진입으로
//        O(N^2) 동기 연쇄가 발생 -> 창 전환 시 "응답없음".
// [해결] DefWindowProc(일반 윈도우 프록)만 직접 호출한다.
//        타이틀바 활성/비활성 렌더링은 유지되고 버튼 순회는 생략된다.
BOOL CShopSetupDlg::OnNcActivate(BOOL bActive)
{
    // DefWindowProc(dialogHwnd) 는 결국 DefDlgProcA 로 라우팅되어
    // xxxSaveDlgFocus -> 버튼 50개 BM_SETSTYLE SendMessage -> O(N^2) 연쇄
    // -> "응답없음" 이 발생한다.
    // TRUE 반환만으로 USER32 가 타이틀바를 다시 그리므로 DefProc 호출 불필요.
    UNREFERENCED_PARAMETER(bActive);
    return TRUE;
}

void CShopSetupDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    // WM_ACTIVATE 도 DefDlgProcA -> xxxSaveDlgFocus -> 버튼 50개 BM_SETSTYLE
    // -> O(N^2) 연쇄 -> "응답없음" 유발.
    // base 클래스(CDialog::OnActivate) 호출 금지.
    UNREFERENCED_PARAMETER(nState);
    UNREFERENCED_PARAMETER(pWndOther);
    UNREFERENCED_PARAMETER(bMinimized);
}

// ============================================================================
// 생성자 / 소멸자
// ============================================================================
CShopSetupDlg::CShopSetupDlg(CWnd* pParent)
    : CDialog(IDD_SHOP_SETUP_DLG, pParent)
    , m_nActiveTab(0)
    , m_uHoverTimer(0)
    , m_nHoverInputId(-1)
{
    m_intPort           = 8002;
    m_intCardTimeout    = 60;
    m_intNoSignAmount   = 50000;
    m_intTaxPercent     = 10;
    m_strCardDetectParam = _T("KFTCOneCAP TEST");
    m_intSignPadPort    = 56;
    m_intScannerPort    = 0;
}

CShopSetupDlg::~CShopSetupDlg()
{
    if (m_fontTitle.GetSafeHandle())      m_fontTitle.DeleteObject();
    if (m_fontSubtitle.GetSafeHandle())   m_fontSubtitle.DeleteObject();
    if (m_fontSection.GetSafeHandle())    m_fontSection.DeleteObject();
    if (m_fontLabel.GetSafeHandle())      m_fontLabel.DeleteObject();
    if (m_fontGroupTitle.GetSafeHandle()) m_fontGroupTitle.DeleteObject();
    if (m_brushBg.GetSafeHandle())        m_brushBg.DeleteObject();
    if (m_brushWhite.GetSafeHandle())     m_brushWhite.DeleteObject();
    if (m_brushTabContent.GetSafeHandle()) m_brushTabContent.DeleteObject();
}

// ============================================================================
// DoDataExchange
// ============================================================================
void CShopSetupDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_COMBO_VAN_SERVER,    m_comboVanServer);
    DDX_Control(pDX, IDC_EDIT_PORT,           m_editPort);
    DDX_Control(pDX, IDC_EDIT_NO_SIGN_AMOUNT, m_editNoSignAmount);
    DDX_Control(pDX, IDC_EDIT_TAX_PERCENT,    m_editTaxPercent);
    DDX_Control(pDX, IDC_EDIT_CARD_TIMEOUT,   m_editCardTimeout);
    DDX_Control(pDX, IDC_EDIT_CARD_DETECT_PARAM, m_editCardDetectParam);
    DDX_Control(pDX, IDC_EDIT_SIGN_PAD_PORT,  m_editSignPadPort);
    DDX_Control(pDX, IDC_EDIT_SCANNER_PORT,   m_editScannerPort);
    DDX_Text(pDX, IDC_EDIT_PORT,           m_intPort);
    DDX_Text(pDX, IDC_EDIT_CARD_TIMEOUT,   m_intCardTimeout);
    DDX_Text(pDX, IDC_EDIT_NO_SIGN_AMOUNT, m_intNoSignAmount);
    DDX_Text(pDX, IDC_EDIT_TAX_PERCENT,    m_intTaxPercent);
    DDX_Control(pDX, IDC_COMBO_CASH_RECEIPT,  m_comboCashReceipt);
    DDX_Control(pDX, IDC_COMBO_INTERLOCK,     m_comboInterlock);
    DDX_Control(pDX, IDC_COMBO_COMM_TYPE,     m_comboCommType);
    DDX_Text(pDX, IDC_EDIT_CARD_DETECT_PARAM, m_strCardDetectParam);
    DDX_Control(pDX, IDC_COMBO_SIGN_PAD_USE,  m_comboSignPadUse);
    DDX_Text(pDX, IDC_EDIT_SIGN_PAD_PORT,     m_intSignPadPort);
    DDX_Control(pDX, IDC_COMBO_SIGN_PAD_SPEED, m_comboSignPadSpeed);
    DDX_Text(pDX, IDC_EDIT_SCANNER_PORT,      m_intScannerPort);
    DDX_Control(pDX, IDC_COMBO_ALARM_POS,     m_comboAlarmPos);
    DDX_Control(pDX, IDC_COMBO_ALARM_SIZE,    m_comboAlarmSize);
    DDX_Control(pDX, IDC_COMBO_CANCEL_KEY,    m_comboCancelKey);
    DDX_Control(pDX, IDC_COMBO_MSR_KEY,       m_comboMSRKey);
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
    int h = 0;
    // combo window height can include drop-list height; use selection-item height instead
    LRESULT ih = ::SendMessage(pCombo->GetSafeHwnd(), CB_GETITEMHEIGHT, (WPARAM)-1, 0);
    if (ih > 0) h = (int)ih + 14; // border/padding
    if (h <= 0) h = rcCombo.Height();
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
BOOL CShopSetupDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    auto S = [&](int v) { return ModernUIDpi::Scale(m_hWnd, v); };

    m_brushBg.CreateSolidBrush(RGB(249, 250, 252));  // 밝은 회색 배경
    m_brushWhite.CreateSolidBrush(RGB(255, 255, 255));
    m_brushTabContent.CreateSolidBrush(RGB(255, 255, 255));  // card white

    InitializeFonts();

    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    // --------------------------------------------------------
    // 탭 컨트롤 생성 (다이얼로그 리소스에 없으므로 동적 생성)
    // --------------------------------------------------------
    m_tabCtrl.Create(this, IDC_TAB_MAIN, CRect(0, 0, 10, 10));

    // Info icon buttons
    auto CreateInfoBtn = [&](CInfoIconButton& btn, UINT id) {
        btn.Create(_T(""), WS_CHILD | BS_OWNERDRAW,
            CRect(0, 0, S(22), S(22)), this, id);
    };
    CreateInfoBtn(m_btnVanInfo,          IDC_BTN_VAN_SERVER_INFO);
    CreateInfoBtn(m_btnPortInfo,         IDC_BTN_PORT_INFO);
    CreateInfoBtn(m_btnTaxPercentInfo,  IDC_BTN_TAX_PERCENT_INFO);
    CreateInfoBtn(m_btnCommTypeInfo,     IDC_BTN_COMM_TYPE_INFO);
    CreateInfoBtn(m_btnCashReceiptInfo,  IDC_BTN_CASH_RECEIPT_INFO);
    CreateInfoBtn(m_btnCardTimeoutInfo,  IDC_BTN_CARD_TIMEOUT_INFO);
    CreateInfoBtn(m_btnInterlockInfo,    IDC_BTN_INTERLOCK_INFO);
CreateInfoBtn(m_btnMultiVoiceInfo,   IDC_BTN_MULTI_VOICE_INFO);
    CreateInfoBtn(m_btnCardDetectInfo,  IDC_BTN_CARD_DETECT_INFO);
    CreateInfoBtn(m_btnScannerUseInfo,  IDC_BTN_SCANNER_USE_INFO);
    CreateInfoBtn(m_btnAutoResetInfo,   IDC_BTN_AUTO_RESET_INFO);
    CreateInfoBtn(m_btnAutoRebootInfo,  IDC_BTN_AUTO_REBOOT_INFO);
    CreateInfoBtn(m_btnAlarmGraphInfo,  IDC_BTN_ALARM_GRAPH_INFO);
    CreateInfoBtn(m_btnAlarmDualInfo,   IDC_BTN_ALARM_DUAL_INFO);
    CreateInfoBtn(m_btnSignPadUseInfo,   IDC_BTN_SIGN_PAD_USE_INFO);
    CreateInfoBtn(m_btnSignPadPortInfo,  IDC_BTN_SIGN_PAD_PORT_INFO);
    CreateInfoBtn(m_btnSignPadSpeedInfo, IDC_BTN_SIGN_PAD_SPEED_INFO);
    CreateInfoBtn(m_btnAlarmSizeInfo,    IDC_BTN_ALARM_SIZE_INFO);
    m_tabCtrl.AddTab(_T("결제 설정"),       0);
    m_tabCtrl.AddTab(_T("장치 정보"),       1);
    m_tabCtrl.AddTab(_T("시스템 설정"),     2);
    m_tabCtrl.AddTab(_T("가맹점 다운로드"), 3);

    InitializeControls();
    LoadOptionsFromRegistry();

    // 다이얼로그 크기
    const int MARGIN_X = S(kTabPadLeft);
    const int LABEL_W  = S(92);
    const int FIELD_W  = S(120);
    const int COL_GAP  = S(16);

    // [TUNE] 가맹점 다운로드 컬럼 폭 (합계가 kDialogMinW 이내여야 함)
    const int sd_padX  = S(10);   // [TUNE] 좌우 여백
    const int sd_gap   = S(8);    // [TUNE] 컬럼 간격
    const int sd_tagW  = S(60);   // [TUNE] 가맹점N 태그 폭
    const int sd_prodW = S(105);  // [TUNE] 단말기 제품번호
    const int sd_bizW  = S(105);  // [TUNE] 사업자번호
    const int sd_pwdW  = S(48);   // [TUNE] 비밀번호
    const int sd_btnW  = S(82);   // [TUNE] 다운로드 버튼
    const int sd_etcW  = S(80);   // [TUNE] 단말기별 가맹점
    const int sd_nameW = S(110);  // [TUNE] 대표 가맹점

    int topContentW  = (LABEL_W + FIELD_W) * 3 + (COL_GAP * 2);
    int topMinW      = (MARGIN_X * 2) + topContentW;
    int shopInnerW   = (sd_padX*2) + sd_tagW + sd_prodW + sd_bizW
                       + sd_pwdW + sd_btnW + sd_etcW + sd_nameW + (sd_gap*6);
    int bottomMinW   = shopInnerW + 2 * MARGIN_X;
    const int kDialogMinW = 760;  // [TUNE] 다이얼로그 최소폭

    int dialogWidth  = max(kDialogMinW, max(topMinW, bottomMinW));
    int dialogHeight = CalculateRequiredHeight();

    SetWindowPos(NULL, 0, 0, dialogWidth, dialogHeight,
        SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();

    ApplyLayout();

    // 하단 Child Dialog 생성
    if (m_staticShopContainer.GetSafeHwnd() && !m_shopDownDlg.GetSafeHwnd())
    {
        if (m_shopDownDlg.Create(CShopDownDlg::IDD, &m_staticShopContainer))
            m_shopDownDlg.ShowWindow(SW_SHOW);
    }
    if (m_shopDownDlg.GetSafeHwnd() && m_staticShopContainer.GetSafeHwnd())
    {
        CRect rcHost;
        m_staticShopContainer.GetClientRect(&rcHost);
        m_shopDownDlg.SetWindowPos(NULL, 0, 0, rcHost.Width(), rcHost.Height(),
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 첫 번째 탭 표시
    m_tabCtrl.SetCurSel(0);
    ShowTab(0);

    Invalidate();
    return TRUE;
}

// ============================================================================
// CalculateRequiredHeight - 탭 UI 기준 높이
// ============================================================================
int CShopSetupDlg::CalculateRequiredHeight()
{
    auto S = [&](int v) { return ModernUIDpi::Scale(m_hWnd, v); };

    // ── 카드 공통 파라미터 (ApplyLayout과 동일 값) ─────────────────
    const int FIELD_H  = S(44);
    const int cOutY    = S(12);   // 카드 외부 상단
    const int cGapY    = S(12);   // 카드 간 간격
    const int cPadY    = S(16);   // 카드 내부 상하
    const int cHdrH    = S(44);   // 카드 헤더 높이
    const int capH     = S(18);   // 라벨 높이
    const int capG     = S(7);    // 라벨→컨트롤 간격
    const int rG       = S(20);   // 행 간격
    auto oneRow = [&](){ return capH + capG + FIELD_H; };
    auto cardH  = [&](int rows, int extraChecks = 0) -> int {
        // rows: 라벨+컨트롤 행 수, extraChecks: 추가 체크박스 행 수
        return cPadY + cHdrH + oneRow()*rows + rG*(rows-1) + FIELD_H*extraChecks + cPadY;
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
        int card1 = cPadY + cHdrH + oneRow()*1 + cPadY;
        int card2 = cPadY + cHdrH + oneRow()*2 + rG + cPadY;
        int card3 = cPadY + cHdrH + FIELD_H + cPadY;
        int h = cOutY + card1 + cGapY + card2 + cGapY + card3;
        maxTabH = max(maxTabH, h);
    }

    // Tab 2: 시스템 설정 (알림창 2행+체크1행 + 시스템 체크1행 + 단축키 1행)
    {
        int card1 = cPadY + cHdrH + oneRow()*2 + rG + FIELD_H + cPadY;  // 알림창
        int card2 = cPadY + cHdrH + FIELD_H + cPadY;                     // 시스템
        int card3 = cPadY + cHdrH + oneRow() + cPadY;                    // 단축키
        int h = cOutY + card1 + cGapY + card2 + cGapY + card3;
        maxTabH = max(maxTabH, h);
    }

    // Tab 3: 가맹점 다운로드
    {
        int h = S(224);
        maxTabH = max(maxTabH, h);
    }

    const int TITLE_AREA   = S(kTabBarTop);
    const int TAB_H        = S(kTabBarH);
    const int PAD_TOP      = S(kTabPadTop);
    const int PAD_BOTTOM   = S(10);
    const int BUTTON_AREA  = S(76);   // [TUNE] 하단 버튼 영역 높이 (버튼H 36 + 상하여백)
    const int CARD_PAD     = S(28);  // [NOTE] 실제로는 CARD_PAD/2만큼 활용

    return CARD_PAD + TITLE_AREA + TAB_H + PAD_TOP + maxTabH + PAD_BOTTOM + BUTTON_AREA;
}

// ============================================================================
// InitializeFonts
// ============================================================================
void CShopSetupDlg::InitializeFonts()
{
    LOGFONT lf = { 0 };
    ::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    lstrcpy(lf.lfFaceName, _T("Malgun Gothic"));

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

    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 13);
    lf.lfWeight = FW_NORMAL;
    m_fontLabel.DeleteObject();
    m_fontLabel.CreateFontIndirect(&lf);

    lf.lfWeight = FW_BOLD;
    m_fontGroupTitle.DeleteObject();
    m_fontGroupTitle.CreateFontIndirect(&lf);

}

// ============================================================================
// InitializeControls
// ============================================================================
void CShopSetupDlg::InitializeControls()
{
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
    m_btnOk.SetUnderlayColor(RGB(255,255,255));
    m_btnCancel.SetUnderlayColor(RGB(255,255,255));

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
    m_editCardDetectParam.SetUnderlayColor(bgColor);
    m_editSignPadPort.SetUnderlayColor(bgColor);
    m_editScannerPort.SetUnderlayColor(bgColor);

    m_staticShopContainer.SubclassDlgItem(IDC_STATIC_RECT, this);
    m_staticShopContainer.ModifyStyle(
        SS_WHITERECT|SS_BLACKRECT|SS_GRAYRECT|SS_WHITEFRAME|SS_BLACKFRAME|SS_GRAYFRAME,
        SS_NOTIFY);

    // 체크박스
    auto SetupTgl = [&](CModernToggleSwitch& sw, int id, LPCTSTR txt, BOOL bOn)
    {
        sw.SubclassDlgItem(id, this);
        sw.SetFont(&m_fontLabel);
        // 기존 체크박스 스타일 제거 + owner-draw 적용
        sw.ModifyStyle(BS_AUTOCHECKBOX|BS_CHECKBOX|BS_3STATE|BS_AUTO3STATE|BS_AUTORADIOBUTTON|BS_RADIOBUTTON, BS_OWNERDRAW);
        sw.ModifyStyle(WS_BORDER, 0);
        sw.ModifyStyleEx(WS_EX_CLIENTEDGE|WS_EX_STATICEDGE, 0);

        
        sw.SetWindowPos(NULL, 0, 0, 0, 0,
            SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
        sw.RedrawWindow(NULL, NULL,
            RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW);
        sw.SetWindowText(txt);
        sw.SetToggled(bOn);
        sw.SetTextSizePx(13);  // fontLabel과 동일 (13px)
        sw.SetNoWrapEllipsis(TRUE);
        sw.SetUnderlayColor(bgColor);
    };
    SetupTgl(m_chkCardDetect,   IDC_CHECK_CARD_DETECT,   _T("우선 거래"),  FALSE);
    SetupTgl(m_chkMultiVoice,   IDC_CHECK_MULTI_VOICE,   _T("멀티패드 음성 출력"), FALSE);
    SetupTgl(m_chkScannerUse,   IDC_CHECK_SCANNER_USE,   _T("스캐너 사용"),    FALSE);
    SetupTgl(m_chkAlarmGraph,   IDC_CHECK_ALARM_GRAPH,   _T("알림창 그림"),    TRUE);
    SetupTgl(m_chkAlarmDual,    IDC_CHECK_ALARM_DUAL,    _T("알림창 듀얼"),    FALSE);
    SetupTgl(m_chkAutoReset,    IDC_CHECK_AUTO_RESET,    _T("자동 재실행"),    TRUE);
    SetupTgl(m_chkAutoReboot,   IDC_CHECK_AUTO_REBOOT,   _T("자동 리부팅"),    TRUE);
    // 콤보박스 초기화
    // 콤보박스 초기화 (Word 스펙 기준 목록)
    FillCombo(m_comboVanServer,   kVanServers,   (int)(sizeof(kVanServers) / sizeof(kVanServers[0])));
    FillCombo(m_comboCashReceipt, kCashReceipt,  (int)(sizeof(kCashReceipt) / sizeof(kCashReceipt[0])));
    FillCombo(m_comboInterlock,   kInterlock,    (int)(sizeof(kInterlock) / sizeof(kInterlock[0])));
    FillCombo(m_comboCommType,    kCommType,     (int)(sizeof(kCommType) / sizeof(kCommType[0])));
    FillCombo(m_comboSignPadUse,  kSignPadUse,   (int)(sizeof(kSignPadUse) / sizeof(kSignPadUse[0])));
    FillCombo(m_comboSignPadSpeed,kSignPadSpeed, (int)(sizeof(kSignPadSpeed) / sizeof(kSignPadSpeed[0])));
    FillCombo(m_comboAlarmPos,    kAlarmPos,     (int)(sizeof(kAlarmPos) / sizeof(kAlarmPos[0])));
    FillCombo(m_comboAlarmSize,   kAlarmSize,    (int)(sizeof(kAlarmSize) / sizeof(kAlarmSize[0])));
    FillCombo(m_comboCancelKey,   kHotkeys,      (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])));
    FillCombo(m_comboMSRKey,      kHotkeys,      (int)(sizeof(kHotkeys) / sizeof(kHotkeys[0])));

    // 라벨 폰트
    const int lblIds[] = {
        IDC_STATIC_VAN_SERVER, IDC_STATIC_PORT,
        IDC_STATIC_CARD_TIMEOUT, IDC_STATIC_NO_SIGN_AMOUNT, IDC_STATIC_TAX_PERCENT,
        IDC_STATIC_CASH_RECEIPT, IDC_STATIC_INTERLOCK,
        IDC_STATIC_COMM_TYPE, IDC_STATIC_SIGN_PAD_USE, IDC_STATIC_SIGN_PAD_PORT,
        IDC_STATIC_SIGN_PAD_SPEED, IDC_STATIC_ALARM_POS, IDC_STATIC_ALARM_SIZE,
        IDC_STATIC_CANCEL_KEY, IDC_STATIC_MSR_KEY
    };
    for (int id : lblIds)
    {
        CWnd* p = GetDlgItem(id);
        if (p) p->SetFont(&m_fontLabel);
    }
}

// --- ScalePx: DPI-aware scaling shorthand ---
int CShopSetupDlg::ScalePx(int px) const
{
    return ModernUIDpi::Scale(m_hWnd, px);
}

// --- MoveCtrl: move a dialog control; ComboBox gets standard drop height ---
void CShopSetupDlg::MoveCtrl(int nID, int x, int y, int w, int h, BOOL bShow)
{
    CWnd* p = GetDlgItem(nID);
    if (!p || !p->GetSafeHwnd()) return;
    TCHAR cls[64] = { 0 };
    ::GetClassName(p->GetSafeHwnd(), cls, 63);
    if (_tcsicmp(cls, _T("ComboBox")) == 0)
    {
        p->MoveWindow(x, y, w, ScalePx(220));
        ::SendMessage(p->GetSafeHwnd(), CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)(h - 2));
        ::SendMessage(p->GetSafeHwnd(), CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)(h - 2));
    }
    else
    {
        p->MoveWindow(x, y, w, h);
    }
    p->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
}
// --- Tab 0: card reader settings ---
void CShopSetupDlg::ApplyLayoutTab0()
{
    auto S    = [&](int v)                              { return ScalePx(v); };
    auto Move = [&](int id, int x, int y, int w, int h) { MoveCtrl(id, x, y, w, h); };
    const int CTRL_H       = S(40);
    const int COMBO_DROP_H = S(220);
    const int FIELD_H      = CTRL_H;
    auto PlaceInfoBtn = [&](CInfoIconButton& btn, int labelId, int lx, int ly, int lcapH) {
     

   if (!btn.GetSafeHwnd()) return;
        const int BtnSz  = S(18);
        const int BtnGap = S(4);
        int bx = lx + BtnGap;
        int by = ly + (lcapH - BtnSz) / 2;
        CWnd* pLbl = GetDlgItem(labelId);
        if (pLbl && pLbl->GetSafeHwnd()) {
            CClientDC cdc(pLbl);
            CFont* pFont = pLbl->GetFont();
            CFont* pOld  = pFont ? cdc.SelectObject(pFont) : NULL;
            CString strLbl;
            pLbl->GetWindowText(strLbl);
            CSize sz = cdc.GetTextExtent(strLbl);
            if (pOld) cdc.SelectObject(pOld);
            bx = lx + sz.cx + BtnGap;
        }
        btn.SetWindowPos(NULL, bx, by, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
    };
    int y = m_rcTabContent.top + S(kTabPadTop);

        const int cardOuterPadX = 16;
        const int cardOuterPadY = 12;
        const int cardGapY      = 16;

        const int cardPadX      = 22;
        const int cardPadY      = 16;   // 상하
        const int headerH       = 44;   // 불릿헤더
        const int capH          = 18;   // [TUNE] 캡션(라벨) 높이
        const int capGap        = 7;    // 캡션간격
        const int rowGap        = 20;   // 행간격
        const int colGap        = 20;

        int cardLeft  = m_rcTabContent.left  + cardOuterPadX;
        int cardRight = m_rcTabContent.right - cardOuterPadX;
        int cardW     = cardRight - cardLeft;

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
        Move(IDC_COMBO_VAN_SERVER,  innerX, y1 + capH + capGap, colW, FIELD_H);

        // Info icon button: right next to label text
        if (m_btnVanInfo.GetSafeHwnd())
        {
            const int INFO_BTN_SZ = S(18);
            const int INFO_BTN_GAP = S(4);
            int btnX = innerX + INFO_BTN_GAP;  // fallback
            int btnY = y1 + (capH - INFO_BTN_SZ) / 2;
            // Measure text width to place button immediately after label
            CWnd* pLbl = GetDlgItem(IDC_STATIC_VAN_SERVER);
            if (pLbl && pLbl->GetSafeHwnd())
            {
                CClientDC cdc(pLbl);
                CFont* pFont = pLbl->GetFont();
                CFont* pOld  = pFont ? cdc.SelectObject(pFont) : NULL;
                CString strLbl;
                pLbl->GetWindowText(strLbl);
                CSize sz = cdc.GetTextExtent(strLbl);
                if (pOld) cdc.SelectObject(pOld);
                btnX = innerX + sz.cx + INFO_BTN_GAP;
            }
            m_btnVanInfo.SetWindowPos(NULL, btnX, btnY, INFO_BTN_SZ, INFO_BTN_SZ,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        // 우: 포트번호
        Move(IDC_STATIC_PORT,       innerX + colW + colGap, y1, colW, capH);
        PlaceInfoBtn(m_btnPortInfo, IDC_STATIC_PORT, innerX + colW + colGap, y1, capH);
        Move(IDC_EDIT_PORT,         innerX + colW + colGap, y1 + capH + capGap, colW, FIELD_H);

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

        // 통신방식 / 우선 거래 설정 (2열)
        colW = (innerW - colGap) / 2;
        Move(IDC_STATIC_COMM_TYPE, innerX, y2, colW, capH);
        PlaceInfoBtn(m_btnCommTypeInfo, IDC_STATIC_COMM_TYPE, innerX, y2, capH);
        Move(IDC_COMBO_COMM_TYPE,  innerX, y2 + capH + capGap, colW, FIELD_H);

        // 우선 거래: 토글(라벨+스위치) + 입력(같은 줄)
        const int inGap = 10;
        int editW = 160;
        if (colW < 280) editW = 120;
        else if (colW < 340) editW = 140;

        int toggleMinW = 130;
        int toggleW = colW - editW - inGap;
        if (toggleW < toggleMinW)
        {
            int need = toggleMinW - toggleW;
            editW -= need;
            if (editW < 90) editW = 90;
            toggleW = colW - editW - inGap;
        }

        int rx = innerX + colW + colGap;
                // 우선 거래: 토글 오른쪽에 팝오버 아이콘 영역 확보(겹치지 않게)
        {
            const int BtnSz  = S(18);
            const int BtnGap = S(4);
            int iconNeed = BtnSz + BtnGap;
            int tW = toggleW;
            int eW = editW;
            // 아이콘 공간을 위해 편집폭에서 먼저 확보
            if (eW > 90 + iconNeed) eW -= iconNeed;
            else if (tW > 70 + iconNeed) tW -= iconNeed;
            Move(IDC_CHECK_CARD_DETECT,      rx,                   y2 + capH + capGap, tW, FIELD_H);
            if (m_btnCardDetectInfo.GetSafeHwnd())
            {
                int ibX = rx + tW + BtnGap;
                int ibY = (y2 + capH + capGap) + (FIELD_H - BtnSz) / 2;
                m_btnCardDetectInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            Move(IDC_EDIT_CARD_DETECT_PARAM, rx + tW + iconNeed + inGap, y2 + capH + capGap, eW, FIELD_H);
        }

        y2 += capH + capGap + FIELD_H + rowGap;

        // 무서명 / 세금 자동 역산 (2열)
        Move(IDC_STATIC_NO_SIGN_AMOUNT, innerX, y2, colW, capH);
        Move(IDC_EDIT_NO_SIGN_AMOUNT,   innerX, y2 + capH + capGap, colW, FIELD_H);

        Move(IDC_STATIC_TAX_PERCENT,    innerX + colW + colGap, y2, colW, capH);
        PlaceInfoBtn(m_btnTaxPercentInfo, IDC_STATIC_TAX_PERCENT, innerX + colW + colGap, y2, capH);
        Move(IDC_EDIT_TAX_PERCENT,      innerX + colW + colGap, y2 + capH + capGap, colW, FIELD_H);

        y2 += capH + capGap + FIELD_H + rowGap;

        // 현금영수증 (1열, 반 폭)
        Move(IDC_STATIC_CASH_RECEIPT, innerX, y2, colW, capH);
        PlaceInfoBtn(m_btnCashReceiptInfo, IDC_STATIC_CASH_RECEIPT, innerX, y2, capH);
        Move(IDC_COMBO_CASH_RECEIPT,  innerX, y2 + capH + capGap, colW, FIELD_H);
        y2 += capH + capGap + FIELD_H;

        int payCardH = (y2 + cardPadY) - curY;
        m_rcCardPayMethod = CRect(cardLeft, curY, cardRight, curY + payCardH);
}

// --- Tab 1+2: devices and system settings ---
void CShopSetupDlg::ApplyLayoutTab1()
{
    auto S    = [&](int v)                              { return ScalePx(v); };
    auto Move = [&](int id, int x, int y, int w, int h) { MoveCtrl(id, x, y, w, h); };
    const int CTRL_H       = S(40);
    const int COMBO_DROP_H = S(220);
    const int FIELD_H      = CTRL_H;
    auto PlaceInfoBtn = [&](CInfoIconButton& btn, int labelId, int lx, int ly, int lcapH) {
     

   if (!btn.GetSafeHwnd()) return;
        const int BtnSz  = S(18);
        const int BtnGap = S(4);
        int bx = lx + BtnGap;
        int by = ly + (lcapH - BtnSz) / 2;
        CWnd* pLbl = GetDlgItem(labelId);
        if (pLbl && pLbl->GetSafeHwnd()) {
            CClientDC cdc(pLbl);
            CFont* pFont = pLbl->GetFont();
            CFont* pOld  = pFont ? cdc.SelectObject(pFont) : NULL;
            CString strLbl;
            pLbl->GetWindowText(strLbl);
            CSize sz = cdc.GetTextExtent(strLbl);
            if (pOld) cdc.SelectObject(pOld);
            bx = lx + sz.cx + BtnGap;
        }
        btn.SetWindowPos(NULL, bx, by, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
    };
    int y = m_rcTabContent.top + S(kTabPadTop);

        const int cOutX  = 16;   // [TUNE] 카드 외부 좌우 여백
        const int cOutY  = 12;   // [TUNE] 카드 외부 상단 여백
        const int cGapY  = 12;   // [TUNE] 카드 간 세로 간격
        const int cPadX  = 22;   // [TUNE] 카드 내부 좌우 여백
        const int cPadY  = 16;   // [TUNE] 카드 내부 상하 여백
        const int cHdrH  = 44;   // [TUNE] 카드 헤더 높이
        const int capH   = 18;   // [TUNE] 라벨 텍스트 높이
        const int capG   = 7;    // [TUNE] 라벨→컨트롤 간격
        const int rG     = 20;   // [TUNE] 카드 내 행 간격
        const int cG     = 18;   // [TUNE] 열 간격
        const int chkW   = 140;  // [TUNE] 체크박스 1개 폭 (3개/행 기준)

        int cLeft  = m_rcTabContent.left  + cOutX;
        int cRight = m_rcTabContent.right - cOutX;
        int cW     = cRight - cLeft;
        int curY   = y + cOutY;
        int inX    = cLeft + cPadX;
        int inW    = cW - cPadX * 2;
        int col2W  = (inW - cG) / 2;  // 2열 분할 폭

    // =================================================================
    // Tab 1: 장치 정보
    // =================================================================

        // ── 카드 1: 리더기 ──────────────────────────────
        {
            int fy = curY + cPadY + cHdrH;
            // 행1: 카드 응답 타임아웃 / 연동 방식 (2열)
            Move(IDC_STATIC_CARD_TIMEOUT, inX,          fy, col2W, capH);
            PlaceInfoBtn(m_btnCardTimeoutInfo, IDC_STATIC_CARD_TIMEOUT, inX, fy, capH);
            Move(IDC_EDIT_CARD_TIMEOUT,   inX,          fy+capH+capG, col2W, FIELD_H);
            Move(IDC_STATIC_INTERLOCK,    inX+col2W+cG, fy, col2W, capH);
            PlaceInfoBtn(m_btnInterlockInfo, IDC_STATIC_INTERLOCK, inX+col2W+cG, fy, capH);
            Move(IDC_COMBO_INTERLOCK,     inX+col2W+cG, fy+capH+capG, col2W, FIELD_H);
            fy += capH + capG + FIELD_H + rG;

            int cardH = (fy + cPadY) - curY;
            m_rcGrpReader = CRect(cLeft, curY, cRight, curY + cardH);
            curY = m_rcGrpReader.bottom + cGapY;
        }

        // ── 카드 2: 서명패드 ──────────────────────────────
        {
            int fy = curY + cPadY + cHdrH;
            // 행1: 서명패드 사용 / 포트번호 (2열)
            Move(IDC_STATIC_SIGN_PAD_USE,  inX,          fy, col2W, capH);
            PlaceInfoBtn(m_btnSignPadUseInfo, IDC_STATIC_SIGN_PAD_USE, inX, fy, capH);
            Move(IDC_COMBO_SIGN_PAD_USE,   inX,          fy+capH+capG, col2W, FIELD_H);
            Move(IDC_STATIC_SIGN_PAD_PORT, inX+col2W+cG, fy, col2W, capH);
            PlaceInfoBtn(m_btnSignPadPortInfo, IDC_STATIC_SIGN_PAD_PORT, inX+col2W+cG, fy, capH);
            Move(IDC_EDIT_SIGN_PAD_PORT,   inX+col2W+cG, fy+capH+capG, col2W, FIELD_H);
            fy += capH + capG + FIELD_H + rG;
            // 행2: 통신속도 (1열, 반폭)
            Move(IDC_STATIC_SIGN_PAD_SPEED, inX, fy, col2W, capH);
            PlaceInfoBtn(m_btnSignPadSpeedInfo, IDC_STATIC_SIGN_PAD_SPEED, inX, fy, capH);
            Move(IDC_COMBO_SIGN_PAD_SPEED,  inX, fy+capH+capG, col2W, FIELD_H);
            fy += capH + capG + FIELD_H;

            int cardH = (fy + cPadY) - curY;
            m_rcGrpSign = CRect(cLeft, curY, cRight, curY + cardH);
            curY = m_rcGrpSign.bottom + cGapY;
        }

        // ── 카드 3: 기타 ────────────────────────────────
        {
            int fy = curY + cPadY + cHdrH;
            // 스캐너 사용: 토글(라벨+스위치) + 포트 입력(같은 줄)
            const int inGap2 = 10;
            int editW2 = 110;
            if (col2W < 260) editW2 = 90;
            int toggleMinW2 = 130;
            int toggleW2 = col2W - editW2 - inGap2;
            if (toggleW2 < toggleMinW2)
            {
                int need = toggleMinW2 - toggleW2;
                editW2 -= need;
                if (editW2 < 70) editW2 = 70;
                toggleW2 = col2W - editW2 - inGap2;
            }

                        // 스캐너 사용: 토글 오른쪽에 팝오버 아이콘 영역 확보(겹치지 않게)
            {
                const int BtnSz  = S(18);
                const int BtnGap = S(4);
                int iconNeed = BtnSz + BtnGap;
                int tW = toggleW2;
                int eW = editW2;
                if (eW > 70 + iconNeed) eW -= iconNeed;
                else if (tW > 60 + iconNeed) tW -= iconNeed;
                Move(IDC_CHECK_SCANNER_USE,   inX,                 fy, tW, FIELD_H);
                if (m_btnScannerUseInfo.GetSafeHwnd())
                {
                    int ibX = inX + tW + BtnGap;
                    int ibY = fy + (FIELD_H - BtnSz) / 2;
                    m_btnScannerUseInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
                }
                Move(IDC_EDIT_SCANNER_PORT,   inX + tW + iconNeed + inGap2, fy, eW, FIELD_H);
            }

            // 멀티보이스 토글(우측, 전체 폭)
                        // 멀티패드 음성 출력: 토글 오른쪽에 팝오버 아이콘 영역 확보(겹치지 않게)
            {
                const int BtnSz  = S(18);
                const int BtnGap = S(6);
                int mvX = inX + col2W + cG;
                int mvW = col2W - (BtnSz + BtnGap);
                if (mvW < S(60)) mvW = max(1, col2W - (BtnSz + BtnGap));
                Move(IDC_CHECK_MULTI_VOICE, mvX, fy, mvW, FIELD_H);
                if (m_btnMultiVoiceInfo.GetSafeHwnd())
                {
                    int ibX = mvX + mvW + BtnGap;
                    int ibY = fy + (FIELD_H - BtnSz) / 2;
                    m_btnMultiVoiceInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }

            fy += FIELD_H;

            int cardH = (fy + cPadY) - curY;
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
            Move(IDC_STATIC_ALARM_SIZE, inX,          fy, col2W, capH);
            PlaceInfoBtn(m_btnAlarmSizeInfo, IDC_STATIC_ALARM_SIZE, inX, fy, capH);
            Move(IDC_COMBO_ALARM_SIZE,  inX,          fy+capH+capG, col2W, FIELD_H);
            Move(IDC_STATIC_ALARM_POS,  inX+col2W+cG, fy, col2W, capH);
            Move(IDC_COMBO_ALARM_POS,   inX+col2W+cG, fy+capH+capG, col2W, FIELD_H);
            fy += capH + capG + FIELD_H + rG;
            // 행2: 체크박스 3개 (그래프/원상복구/듀얼모니터)
                        int chk2W = (inW - cG) / 2;
            // 알림창 옵션: 토글 오른쪽에 팝오버 아이콘 영역 확보(겹치지 않게)
            {
                const int BtnSz  = S(18);
                const int BtnGap = S(4);
                int iconNeed = BtnSz + BtnGap;
                int wL = chk2W;
                int wR = chk2W;
                if (wL > S(80) + iconNeed) wL -= iconNeed;
                if (wR > S(80) + iconNeed) wR -= iconNeed;
                int xL = inX;
                int xR = inX + chk2W + cG;
                Move(IDC_CHECK_ALARM_GRAPH, xL, fy, wL, FIELD_H);
                if (m_btnAlarmGraphInfo.GetSafeHwnd())
                {
                    int ibX = xL + wL + BtnGap;
                    int ibY = fy + (FIELD_H - BtnSz) / 2;
                    m_btnAlarmGraphInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
                }
                Move(IDC_CHECK_ALARM_DUAL,  xR, fy, wR, FIELD_H);
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
            // 체크박스 2개: 자동 리셋 / 자동 재부팅
                        int chk2W = (inW - cG) / 2;
            // 자동 옵션: 토글 오른쪽에 팝오버 아이콘 영역 확보(겹치지 않게)
            {
                const int BtnSz  = S(18);
                const int BtnGap = S(4);
                int iconNeed = BtnSz + BtnGap;
                int wL = chk2W;
                int wR = chk2W;
                if (wL > S(80) + iconNeed) wL -= iconNeed;
                if (wR > S(80) + iconNeed) wR -= iconNeed;
                int xL = inX;
                int xR = inX + chk2W + cG;
                Move(IDC_CHECK_AUTO_RESET,  xL, fy, wL, FIELD_H);
                if (m_btnAutoResetInfo.GetSafeHwnd())
                {
                    int ibX = xL + wL + BtnGap;
                    int ibY = fy + (FIELD_H - BtnSz) / 2;
                    m_btnAutoResetInfo.SetWindowPos(NULL, ibX, ibY, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
                }
                Move(IDC_CHECK_AUTO_REBOOT, xR, fy, wR, FIELD_H);
                if (m_btnAutoRebootInfo.GetSafeHwnd())
                {
                    int ibX = xR + wR + BtnGap;
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
            Move(IDC_STATIC_CANCEL_KEY, inX,          fy, col2W, capH);
            Move(IDC_COMBO_CANCEL_KEY,  inX,          fy+capH+capG, col2W, FIELD_H);
            Move(IDC_STATIC_MSR_KEY,    inX+col2W+cG, fy, col2W, capH);
            Move(IDC_COMBO_MSR_KEY,     inX+col2W+cG, fy+capH+capG, col2W, FIELD_H);
            fy += capH + capG + FIELD_H;

            int cardH = (fy + cPadY) - curY;
            m_rcGrpHotkey = CRect(cLeft, curY, cRight, curY + cardH);
            // curY = m_rcGrpHotkey.bottom + cGapY;  // Tab2 끝
        }
}

// --- Tab 3: merchant download ---
void CShopSetupDlg::ApplyLayoutTab3()
{
    auto S = [&](int v) { return ScalePx(v); };
    CRect rc;
    GetClientRect(&rc);
    int y = m_rcTabContent.top + S(kTabPadTop);

        // Tab1/2와 동일한 카드 외부/내부 파라미터를 사용해 정렬을 맞춘다.
        const int cOutX  = 16;   // 카드 외부 좌우 여백
        const int cOutY  = 12;   // 카드 외부 상단 여백
        const int cPadX  = 22;   // 카드 내부 좌우 여백
        const int cPadY  = 16;   // 카드 내부 상하 여백
        const int cHdrH  = 44;   // 카드 헤더 높이(세로선/타이틀 영역)
        const int hostGapBottom = 14;
        const int cardBottomPad = 18;

        const int BUTTON_H      = 36;
        const int BUTTON_BOTTOM = 22;

        // 하단 버튼 윗쪽까지를 카드 영역으로 사용
        int btnY = rc.bottom - (cardBottomPad + BUTTON_BOTTOM + BUTTON_H);
        int cardLeft  = m_rcTabContent.left  + ModernUIDpi::Scale(m_hWnd, cOutX);
        int cardRight = m_rcTabContent.right - ModernUIDpi::Scale(m_hWnd, cOutX);
        int cardTop   = y + ModernUIDpi::Scale(m_hWnd, cOutY);
        int cardBot   = btnY - ModernUIDpi::Scale(m_hWnd, cardBottomPad);
        if (cardBot < cardTop + ModernUIDpi::Scale(m_hWnd, 240))
            cardBot = cardTop + ModernUIDpi::Scale(m_hWnd, 240);

        m_rcCardShopDown = CRect(cardLeft, cardTop, cardRight, cardBot);

        // Child(ShopDownDlg)는 카드 내부 컨텐츠 영역(헤더 아래)만 차지하도록 한다.
        CRect rcHost(
            cardLeft + ModernUIDpi::Scale(m_hWnd, cPadX),
            cardTop  + ModernUIDpi::Scale(m_hWnd, cHdrH + cPadY),
            cardRight - ModernUIDpi::Scale(m_hWnd, cPadX),
            cardBot  - ModernUIDpi::Scale(m_hWnd, cPadY + hostGapBottom));

        if (rcHost.Height() < ModernUIDpi::Scale(m_hWnd, 200))
            rcHost.bottom = rcHost.top + ModernUIDpi::Scale(m_hWnd, 200);

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
    CRect rc;
    GetClientRect(&rc);

    auto S = [&](int v) { return ModernUIDpi::Scale(m_hWnd, v); };

    // 헬퍼: 라벨 텍스트 오른쪽에 인포 아이콘 버튼 배치
    auto PlaceInfoBtn = [&](CInfoIconButton& btn, int labelId, int lx, int ly, int lcapH) {
     

   if (!btn.GetSafeHwnd()) return;
        const int BtnSz  = S(18);
        const int BtnGap = S(4);
        int bx = lx + BtnGap;
        int by = ly + (lcapH - BtnSz) / 2;
        CWnd* pLbl = GetDlgItem(labelId);
        if (pLbl && pLbl->GetSafeHwnd()) {
            CClientDC cdc(pLbl);
            CFont* pFont = pLbl->GetFont();
            CFont* pOld  = pFont ? cdc.SelectObject(pFont) : NULL;
            CString strLbl;
            pLbl->GetWindowText(strLbl);
            CSize sz = cdc.GetTextExtent(strLbl);
            if (pOld) cdc.SelectObject(pOld);
            bx = lx + sz.cx + BtnGap;
        }
        btn.SetWindowPos(NULL, bx, by, BtnSz, BtnSz, SWP_NOZORDER | SWP_NOACTIVATE);
    };

    const int MARGIN   = S(kTabPadLeft);
    const int LABEL_W  = S(92);
    const int FIELD_W  = S(120);
    const int LF_GAP   = S(8);
    const int FIELD_W_IN = FIELD_W - LF_GAP;
    const int CTRL_H   = S(40);   // [TUNE] 컨트롤 시각적 높이 (Edit/Combo 동일)
    const int FIELD_H  = CTRL_H;  // 하위 호환용 alias
    const int COMBO_DROP_H = S(220); // [TUNE] combo drop list height
    const int ROW_GAP  = S(16);
    const int COL_GAP  = S(16);
    const int GROUP_H  = S(kGroupTitleH);
    const int GROUP_GAP = S(kGroupGapBelowTitle);
    const int NEXT_GRP = S(kGapToNextGroup);

    const int labelOffset = (FIELD_H - S(20)) / 2; // label vertical align

    // ---- 탭 컨트롤 위치 ----
    // 탭 바를 타이틀/서브타이틀 구분선 바로 아래 배치
    const int TAB_INSET = S(2); // keep tab visuals from touching outer card border
    int tabLeft   = S(20) + TAB_INSET;
    int tabRight  = rc.Width() - S(20) - TAB_INSET;
    int tabTop    = S(kTabBarTop);
    int tabBottom = tabTop + S(kTabBarH) + S(200); // 탭 컨트롤 전체 높이(내부 클라이언트 포함)
    int tabH = S(CModernTabCtrl::kBarH) + S(8); // 탭 바 높이 + 여백
    m_tabCtrl.MoveWindow(tabLeft, tabTop, tabRight - tabLeft, tabH);

    // 탭 컨텐츠 영역: 탭 바 바로 아래부터
    m_rcTabContent = CRect(tabLeft, tabTop + tabH, tabRight, rc.bottom - S(90));

    // ---- 컨텐츠 영역 기준 좌표 ----
    int contentLeft = m_rcTabContent.left + (MARGIN - tabLeft);
    int x  = max(contentLeft, MARGIN);
    int x1 = x;
    int x2 = x1 + LABEL_W + FIELD_W + COL_GAP;
    int x3 = x2 + LABEL_W + FIELD_W + COL_GAP;

    // 모든 탭의 컨텐츠는 동일한 Y 기준에서 배치 (ShowTab이 show/hide)
    int y = m_rcTabContent.top + S(kTabPadTop);

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
            ::SendMessage(p->GetSafeHwnd(), CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)(CTRL_H - 2));
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
        const int BUTTON_H      = 36;    // [TUNE] 버튼 높이
        const int BUTTON_BOTTOM = 18;   // [TUNE] 버튼 하단 여백
        const int BUTTON_GAP    = 8;    // [TUNE] 버튼 간격
        const int BUTTON_W      = 110;  // [TUNE] 버튼 폭
        // btnY: 다이얼로그 하단에서 역산 (CARD_PAD=메인카드 하단 여백 포함)
        int btnY = rc.bottom - (22 + BUTTON_BOTTOM + BUTTON_H);
        int btnX = rc.Width() / 2;
        m_btnOk.MoveWindow(btnX - BUTTON_W - BUTTON_GAP/2, btnY, BUTTON_W, BUTTON_H);
        m_btnCancel.MoveWindow(btnX + BUTTON_GAP/2,         btnY, BUTTON_W, BUTTON_H);
        m_btnOk.ShowWindow(SW_SHOW);
        m_btnCancel.ShowWindow(SW_SHOW);
    }
}

void CShopSetupDlg::LoadOptionsFromRegistry()
{
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
        m_intTaxPercent = 0; // 기본값

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
    m_chkAutoReset.SetToggled(ReadToggle_DefaultOnWhenMissing(AUTO_RESTART_FIELD, TRUE, _T("0"), _T("1")));
    m_chkAutoReboot.SetToggled(ReadToggle_DefaultOnWhenMissing(AUTO_REBOOT_FIELD, TRUE, _T("0"), _T("1")));

    // UI에 반영
    UpdateData(FALSE);
    UpdateToggleDependentEdits(FALSE);
}

// ============================================================================
// SaveOptionsToRegistry - OK 버튼에서 일괄 저장
// ============================================================================
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
    AfxGetApp()->WriteProfileString(SEC_SERIALPORT, AUTO_RESTART_FIELD, m_chkAutoReset.IsToggled() ? _T("0") : _T("1"));
    AfxGetApp()->WriteProfileString(SEC_SERIALPORT, AUTO_REBOOT_FIELD, m_chkAutoReboot.IsToggled() ? _T("0") : _T("1"));
}

// ============================================================================
// ShowTab - 탭 전환 시 컨트롤 show/hide
// ============================================================================
void CShopSetupDlg::ShowTab(int nTab)
{
    m_nActiveTab = nTab;

    // Close popover on tab switch
    if (m_popover.GetSafeHwnd()) m_popover.Hide();

    // [1.7] 탭 전환 플리커 최소화: redraw 일시 중지
    SetRedraw(FALSE);
    if (m_tabCtrl.GetSafeHwnd()) m_tabCtrl.SetRedraw(FALSE);

    // Tab 0: 결제 설정
    //   [서버 설정] : 금융결제원 서버 / 포트번호
    //   [결제 방식] : 통신방식 / 우선 거래 설정 / 무서명 기준금액 / 세금 자동 역산 / 현금영수증
    static const int s_tab0[] = {
        // 서버 설정
        IDC_STATIC_VAN_SERVER, IDC_COMBO_VAN_SERVER,
        IDC_STATIC_PORT,       IDC_EDIT_PORT,
        // 결제 방식
        IDC_STATIC_COMM_TYPE,  IDC_COMBO_COMM_TYPE,
        IDC_CHECK_CARD_DETECT, IDC_EDIT_CARD_DETECT_PARAM,
        IDC_STATIC_NO_SIGN_AMOUNT, IDC_EDIT_NO_SIGN_AMOUNT,
        IDC_STATIC_TAX_PERCENT,    IDC_EDIT_TAX_PERCENT,
        IDC_STATIC_CASH_RECEIPT,   IDC_COMBO_CASH_RECEIPT,
        0
    };

    // Tab 1: 장치 정보 (결제 방식 제외)
    static const int s_tab1[] = {
        IDC_STATIC_CARD_TIMEOUT,   IDC_EDIT_CARD_TIMEOUT,
        IDC_STATIC_INTERLOCK,      IDC_COMBO_INTERLOCK,
        IDC_STATIC_SIGN_PAD_USE,   IDC_COMBO_SIGN_PAD_USE,
        IDC_STATIC_SIGN_PAD_PORT,  IDC_EDIT_SIGN_PAD_PORT,
        IDC_STATIC_SIGN_PAD_SPEED, IDC_COMBO_SIGN_PAD_SPEED,
        IDC_CHECK_SCANNER_USE,     IDC_EDIT_SCANNER_PORT,
        IDC_CHECK_MULTI_VOICE,
        0
    };

    // Tab 2: 시스템 설정
    static const int s_tab2[] = {
        IDC_STATIC_ALARM_SIZE,  IDC_COMBO_ALARM_SIZE,
        IDC_STATIC_ALARM_POS,   IDC_COMBO_ALARM_POS,
        IDC_CHECK_ALARM_GRAPH, IDC_CHECK_ALARM_DUAL,
        IDC_CHECK_AUTO_RESET,  IDC_CHECK_AUTO_REBOOT,
        IDC_STATIC_CANCEL_KEY,  IDC_COMBO_CANCEL_KEY,
        IDC_STATIC_MSR_KEY,     IDC_COMBO_MSR_KEY,
        0
    };

    const int* tabs[3] = { s_tab0, s_tab1, s_tab2 };

    // 모든 컨트롤 숨기기
    for (int t = 0; t < 3; t++)
    {
        for (int i = 0; tabs[t][i]; i++)
        {
            CWnd* p = GetDlgItem(tabs[t][i]);
            if (p) p->ShowWindow(SW_HIDE);
        }
    }
    if (m_staticShopContainer.GetSafeHwnd())
        m_staticShopContainer.ShowWindow(SW_HIDE);

    // 활성 탭 컨트롤만 표시
    if (nTab >= 0 && nTab < 3)
    {
        for (int i = 0; tabs[nTab][i]; i++)
        {
            CWnd* p = GetDlgItem(tabs[nTab][i]);
            if (p) p->ShowWindow(SW_SHOW);
        }
    }
    else if (nTab == 3)
    {
        if (m_staticShopContainer.GetSafeHwnd())
            m_staticShopContainer.ShowWindow(SW_SHOW);
    }

    // Info button visibility per tab
    if (m_btnVanInfo.GetSafeHwnd())
        m_btnVanInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    if (m_btnPortInfo.GetSafeHwnd())
        m_btnPortInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    if (m_btnTaxPercentInfo.GetSafeHwnd())
        m_btnTaxPercentInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    if (m_btnCommTypeInfo.GetSafeHwnd())
        m_btnCommTypeInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    if (m_btnCashReceiptInfo.GetSafeHwnd())
        m_btnCashReceiptInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    if (m_btnCardTimeoutInfo.GetSafeHwnd())
        m_btnCardTimeoutInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    if (m_btnInterlockInfo.GetSafeHwnd())
        m_btnInterlockInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    if (m_btnMultiVoiceInfo.GetSafeHwnd())
        m_btnMultiVoiceInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    if (m_btnScannerUseInfo.GetSafeHwnd())
        m_btnScannerUseInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    if (m_btnCardDetectInfo.GetSafeHwnd())
        m_btnCardDetectInfo.ShowWindow(nTab == 0 ? SW_SHOW : SW_HIDE);
    if (m_btnAlarmGraphInfo.GetSafeHwnd())
        m_btnAlarmGraphInfo.ShowWindow(nTab == 2 ? SW_SHOW : SW_HIDE);
    if (m_btnAlarmDualInfo.GetSafeHwnd())
        m_btnAlarmDualInfo.ShowWindow(nTab == 2 ? SW_SHOW : SW_HIDE);
    if (m_btnAutoResetInfo.GetSafeHwnd())
        m_btnAutoResetInfo.ShowWindow(nTab == 2 ? SW_SHOW : SW_HIDE);
    if (m_btnAutoRebootInfo.GetSafeHwnd())
        m_btnAutoRebootInfo.ShowWindow(nTab == 2 ? SW_SHOW : SW_HIDE);
    if (m_btnSignPadUseInfo.GetSafeHwnd())
        m_btnSignPadUseInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    if (m_btnSignPadPortInfo.GetSafeHwnd())
        m_btnSignPadPortInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    if (m_btnSignPadSpeedInfo.GetSafeHwnd())
        m_btnSignPadSpeedInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
    if (m_btnAlarmSizeInfo.GetSafeHwnd())
        m_btnAlarmSizeInfo.ShowWindow(nTab == 2 ? SW_SHOW : SW_HIDE);

    // [1.7] redraw 재개 후, 탭 영역(탭바+컨텐츠)만 갱신
    if (m_tabCtrl.GetSafeHwnd()) m_tabCtrl.SetRedraw(TRUE);
    SetRedraw(TRUE);

    CRect rcRedraw;
    GetClientRect(&rcRedraw);
    rcRedraw.top = kTabBarTop - 2; // 탭바 포함(살짝 위로)
    RedrawWindow(&rcRedraw, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

// ============================================================================
// TCN_SELCHANGE 핸들러
// ============================================================================
void CShopSetupDlg::OnTcnSelchange(NMHDR* pNMHDR, LRESULT* pResult)
{
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
        }
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
void CShopSetupDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(&rc);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    DrawBackground(&memDC);

    // ── 헤더: 배지 아이콘 + 타이틀 + 서브타이틀 ───────────────────
    {
        HDC hdc2 = memDC.GetSafeHdc();
        ModernUIGfx::EnsureGdiplusStartup();
        Gdiplus::Graphics gh(hdc2);
        gh.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        gh.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

        // 배지 배경 (kHdrBadgeSz x kHdrBadgeSz, 파랑 그라디언트)
        const float bx = (float)kHdrBadgeX;
        const float by = (float)kHdrBadgeY;
        const float bsz = (float)kHdrBadgeSz;
        {
            auto MRR = [](Gdiplus::GraphicsPath& path, Gdiplus::RectF r, float rad)
            {
                const float d = rad * 2.0f;
                Gdiplus::RectF a(r.X, r.Y, d, d);
                path.AddArc(a, 180, 90); a.X = r.X + r.Width - d;
                path.AddArc(a, 270, 90); a.Y = r.Y + r.Height - d;
                path.AddArc(a,   0, 90); a.X = r.X;
                path.AddArc(a,  90, 90); path.CloseFigure();
            };
            Gdiplus::GraphicsPath bp;
            MRR(bp, Gdiplus::RectF(bx, by, bsz, bsz), 9.0f);
            Gdiplus::LinearGradientBrush bg(
                Gdiplus::PointF(bx, by), Gdiplus::PointF(bx, by+bsz),
                Gdiplus::Color(255, 60, 130, 245),
                Gdiplus::Color(255, 28,  76, 210));
            gh.FillPath(&bg, &bp);

            // Modern store icon (Toss/Kakao/Naver style - white filled building silhouette)
            const float cx=bx+bsz*0.5f, cy=by+bsz*0.5f;
            Gdiplus::SolidBrush wBr(Gdiplus::Color(255,255,255,255));
            const float iH=bsz*0.60f, iW=bsz*0.52f;
            const float rH=iH*0.28f, bHh=iH-rH;
            const float iX=cx-iW*0.5f, iY=cy-iH*0.5f+0.5f;
            Gdiplus::PointF rf[3];
            rf[0]=Gdiplus::PointF(cx,iY);
            rf[1]=Gdiplus::PointF(iX-iW*0.14f,iY+rH+1.5f);
            rf[2]=Gdiplus::PointF(iX+iW*1.14f,iY+rH+1.5f);
            gh.FillPolygon(&wBr,rf,3);
            gh.FillRectangle(&wBr,Gdiplus::RectF(iX,iY+rH,iW,bHh));
            Gdiplus::SolidBrush dBr(Gdiplus::Color(130,28,76,210));
            const float dW=iW*0.26f, dH=bHh*0.44f;
            gh.FillRectangle(&dBr,Gdiplus::RectF(cx-dW*0.5f,iY+rH+bHh-dH,dW,dH));
        }

        // 타이틀 (GDI+ ClearType)
        Gdiplus::FontFamily ff(L"Malgun Gothic");
        Gdiplus::Font fTitle(&ff, 16.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::Font fSub(&ff, 11.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush bTitle(Gdiplus::Color(255, 18, 24, 40));
        Gdiplus::SolidBrush bSub(Gdiplus::Color(255, 130, 142, 162));
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentNear);
        sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

        const float tx = bx + bsz + 12.0f;
        // 타이틀: 배지 세로 중앙 기준 위쪽 절반
        const float titleY = by + bsz * 0.5f - 22.0f;
        gh.DrawString(L"가맹점 설정", -1, &fTitle,
            Gdiplus::RectF(tx, titleY, 300.0f, 24.0f), &sf, &bTitle);
        gh.DrawString(L"가맹점 및 서버 연결 설정을 관리합니다", -1, &fSub,
            Gdiplus::RectF(tx, titleY+26.0f, 360.0f, 16.0f), &sf, &bSub);

        // 구분선 (kHdrDividerY 기준)
        Gdiplus::Pen divPen(Gdiplus::Color(255, 228, 232, 240), 1.0f);
        gh.DrawLine(&divPen,
            Gdiplus::PointF((float)kHdrBadgeX, (float)kHdrDividerY),
            Gdiplus::PointF((float)(rc.Width()-kHdrBadgeX), (float)kHdrDividerY));
    }

    CFont* pOldFont = memDC.SelectObject(&m_fontTitle);
    CPen linePen(PS_SOLID, 1, RGB(228, 232, 240));
    CPen* pOldPen = memDC.SelectObject(&linePen);

    // 그룹 소제목 (활성 탭에 따라)
    DrawGroupLabels(&memDC);

    memDC.SelectObject(pOldPen);
    memDC.SelectObject(pOldFont);

    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

// ============================================================================
// DrawGroupLabels - 활성 탭의 그룹 소제목([결제 방식] 등) 그리기
// ============================================================================
void CShopSetupDlg::DrawGroupLabels(CDC* pDC)
{
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
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(rc, RGB(249, 250, 252));  // 밝은 회색 배경

    const int kCardMarginL = 20;
    const int kCardMarginT = 10;
    const int kCardMarginR = 20;
    const int kCardMarginB = 20;
    const float kRadius = 12.0f;

    CRect contentRect(kCardMarginL, kCardMarginT,
                      rc.Width() - kCardMarginR, rc.bottom - kCardMarginB);

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
            path.AddArc(a,   0, 90); a.X = r.X;
            path.AddArc(a,  90, 90); path.CloseFigure();
        };

        auto DrawMinCard = [&](const CRect& rcSec, const wchar_t* title)
        {
            if (rcSec.IsRectEmpty()) return;
            const float crad = 12.0f;
            const float hdrH = 44.0f;

            Gdiplus::RectF cr(
                (float)rcSec.left,  (float)rcSec.top,
                (float)rcSec.Width(), (float)rcSec.Height());

            // 그림자 2단계
            for (int sh = 2; sh >= 1; sh--) {
                Gdiplus::RectF sr(cr.X, cr.Y+(float)sh, cr.Width, cr.Height);
                Gdiplus::GraphicsPath sp; RR(sp, sr, crad);
                Gdiplus::SolidBrush sb(Gdiplus::Color((BYTE)(sh==2?8:16), 20,40,80));
                g.FillPath(&sb, &sp);
            }

            // 카드 본체 흰색
            Gdiplus::GraphicsPath cp; RR(cp, cr, crad);
            Gdiplus::SolidBrush cf(Gdiplus::Color(255,250,251,253));  // 그룹 배경(#FAFBFD)
            g.FillPath(&cf, &cp);
            // 헤더 구분선
            Gdiplus::Pen hl(Gdiplus::Color(255,238,241,247), 1.0f);
            g.DrawLine(&hl,
                Gdiplus::PointF(cr.X+16.0f, cr.Y+hdrH),
                Gdiplus::PointF(cr.X+cr.Width-16.0f, cr.Y+hdrH));

            // 불릿 + 타이틀
            const float barX  = cr.X + 16.0f;       // 카드 좌측 내부 여백
            const float barW  = 4.0f;               // 세로 바 폭
            const float barH  = 16.0f;              // 세로 바 높이
            const float barY  = cr.Y + (hdrH - barH) * 0.5f;  // 헤더 세로 중앙
            const float barR  = 2.0f;               // 모서리 라운드 반경

            // 세로 accent bar (라운드 사각형)
            Gdiplus::GraphicsPath barPath;
            const float bd = barR * 2.0f;
            barPath.AddArc(barX,          barY,          bd, bd, 180, 90);
            barPath.AddArc(barX+barW-bd,  barY,          bd, bd, 270, 90);
            barPath.AddArc(barX+barW-bd,  barY+barH-bd,  bd, bd,   0, 90);
            barPath.AddArc(barX,          barY+barH-bd,  bd, bd,  90, 90);
            barPath.CloseFigure();
            Gdiplus::SolidBrush barBr(Gdiplus::Color(255, 0, 96, 210));
            g.FillPath(&barBr, &barPath);

            // 타이틀 (세로 바 오른쪽에 10px 간격)
            const float titleX = barX + barW + 10.0f;
            Gdiplus::FontFamily ff2(L"Malgun Gothic");
            Gdiplus::Font fT(&ff2, 12.5f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::SolidBrush bT(Gdiplus::Color(255, 26, 32, 44));
            Gdiplus::StringFormat sf2;
            sf2.SetAlignment(Gdiplus::StringAlignmentNear);
            sf2.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            g.DrawString(title, -1, &fT,
                Gdiplus::RectF(titleX, cr.Y, cr.Width - (titleX - cr.X) - 16.0f, hdrH),
                &sf2, &bT);
        };

        // ── 탭별 렌더링 ────────────────────────────────────────────────
        if (m_nActiveTab == 0)
        {
            DrawMinCard(m_rcCardServer,    L"서버 설정");
            DrawMinCard(m_rcCardPayMethod, L"결제 방식");
        }
        else if (m_nActiveTab == 1)
        {
            DrawMinCard(m_rcGrpReader, L"리더기");
            DrawMinCard(m_rcGrpSign,   L"서명패드");
            DrawMinCard(m_rcGrpEtc,    L"기타");
        }
        else if (m_nActiveTab == 2)
        {
            DrawMinCard(m_rcGrpAlarm,  L"알림창 설정");
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
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

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
    if (m_popover.IsVisible()) m_popover.Hide();
    CDialog::OnDestroy();
}

// ============================================================================
// OnOK / OnCancel
// ============================================================================
void CShopSetupDlg::OnOK()
{
    if (m_popover.GetSafeHwnd()) m_popover.Hide();

    // OK 버튼에서 일괄 저장 (Word 스펙)
    SaveOptionsToRegistry();

    CDialog::OnOK();
}

void CShopSetupDlg::OnCancel()
{
    if (m_popover.GetSafeHwnd()) m_popover.Hide();
    CDialog::OnCancel();
}

// ============================================================================
// DrawInputBorders (하위 호환 stub)
// ============================================================================
void CShopSetupDlg::DrawInputBorders() {}

// ============================================================================
// OnBnClickedVanServerInfo - toggle popover
// ============================================================================
void CShopSetupDlg::OnBnClickedVanServerInfo()
{
    if (m_popover.IsVisible())
    {
        m_popover.Hide();
        return;
    }
    CRect rc;
    m_btnVanInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("금융결제원 서버"),
        _T("금융결제원 서버 선택\n· 실제 거래 서버 : 운영 환경\n· 테스트 서버 : 승인 테스트용\n· 테스트 서버(내부용) : 개발/검증용"),
        this);
}
// ============================================================================
// 팝오버 아이콘 핸들러 - 탭0
// ============================================================================
void CShopSetupDlg::OnBnClickedPortInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnPortInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("포트번호"),
        _T("금융결제원 서버 접속 포트번호\n· 기본값 : 8002"),
        this);
}
void CShopSetupDlg::OnBnClickedCommTypeInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnCommTypeInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("통신방식"),
        _T("포스 프로그램 통신 방식 선택\n· CS 방식: 윈도우 포스 프로그램 (기본값)\n· WEB 방식: WEB 포스 프로그램 (EASYPOS 포함)"),
        this);
}
void CShopSetupDlg::OnBnClickedCashReceiptInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnCashReceiptInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("현금영수증 거래"),
        _T("현금영수증 승인시 입력 방식 선택\n· PINPAD/KEYIN : PINPAD/KEYIN 동시 입력 (기본값)\n· MS : MS 카드 입력\n· KEYIN : KEYIN 입력"),
        this);
}

void CShopSetupDlg::OnBnClickedTaxPercentInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnTaxPercentInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("세금 자동역산 설정"),
        _T("세금 자동 계산 비율 (%)\n· 기본값: 0 (0=세금 없음, 10=공급가액에서 10% 역산)\n※ POS에서 세금 필드를 채우지 않는 경우에만 적용"),
        this);
}

// ============================================================================
// 팝오버 아이콘 핸들러 - 탭1
// ============================================================================
void CShopSetupDlg::OnBnClickedCardTimeoutInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnCardTimeoutInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("카드입력 Timeout"),
        _T("카드 입력 대기 시간 (초 단위)\n· 권장값: 100초 / 0 입력 시 자동 100초 설정"),
        this);
}
void CShopSetupDlg::OnBnClickedInterlockInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnInterlockInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("장치 연동 방식"),
        _T("카드 리더기 연동 방식 선택\n· IC/MS 리더기: 일반 리더기 (기본값)\n· LockType리더기(TDR): TDR 방식 리더기\n· AutoDriven리더기(TTM): TTM 방식 리더기\n· 단말기(forPOS): 단말기 연동 거래\n· 멀티패드(동반위): 멀티패드 및 신형 리더기 사용 (권장값)\n· 멀티패드(씨큐프라임용): 씨큐프라임 포스 전용\n· 멀티패드(키오스크): 사용 중지\n· AOP 리더기: AOP 리더기(Naver Connect 포함)"),
        this);
}
void CShopSetupDlg::OnBnClickedMultiVoiceInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnMultiVoiceInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("음성출력"),
        _T("카드 리딩 시 음성 출력 여부\n· 기본값 : 미사용\n※ SPAY-8800Q, DP636X 모델만 가능"),
        this);
}
void CShopSetupDlg::OnBnClickedCardDetectInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnCardDetectInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("우선 거래 설정"),
        _T("카드 우선 거래 설정\n· 기본값 : 미사용\n입력창에는 POS 프로그램 정보 입력(POS 프로그램 업체 안내 필요)\n※우선 거래가 개발된 POS 프로그램만 사용"),
        this);
}
void CShopSetupDlg::OnBnClickedScannerUseInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnScannerUseInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("스캐너 사용"),
        _T("스캐너 사용 여부 설정\n· 기본값 : 미사용\n입력창에는 포트번호 입력\n※ KFTCOneCAP에서 외부 스캐너를 연동하는 경우 사용"),
        this);
}
void CShopSetupDlg::OnBnClickedAutoResetInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnAutoResetInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("자동 재실행"),
        _T("KFTCOneCAP 종료 시 자동 재실행 여부\n· 기본값 : 사용"),
        this);
}
void CShopSetupDlg::OnBnClickedAutoRebootInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnAutoRebootInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("자동 리부팅"),
        _T("특정 조건에서 단말/PC를 자동 리부팅할지 설정합니다.\n- 사용: 자동 리부팅\n- 미사용: 리부팅하지 않음"),
        this);
}
void CShopSetupDlg::OnBnClickedAlarmGraphInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnAlarmGraphInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("알림창 그림"),
        _T("거래 알림창 이미지 출력 여부\n· 기본값: 사용"),
        this);
}
void CShopSetupDlg::OnBnClickedAlarmDualInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnAlarmDualInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("알림창 듀얼"),
        _T("듀얼 모니터 사용 시 서브 모니터에 알림창 출력\n· 기본값: 미사용"),
        this);
}
void CShopSetupDlg::OnBnClickedSignPadUseInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnSignPadUseInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("서명패드 사용"),
        _T("서명패드 사용여부 설정\n· 예 : 서명패드를 사용하는 경우\n· 아니오 : 서명패드를 사용하지 않는 경우\n· 자체서명 : 포스 화면에서 서명 입력"),
        this);
}

void CShopSetupDlg::OnBnClickedSignPadPortInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnSignPadPortInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("서명패드 포트번호"),
        _T("서명패드가 연결된 COM 포트번호"),
        this);
}

void CShopSetupDlg::OnBnClickedSignPadSpeedInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnSignPadSpeedInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("서명패드 속도"),
        _T("서명패드 통신 속도 선택\n· 115200bps: 멀티패드 사용 시\n· 57600bps: 서명패드 사용 시"),
        this);
}
// ============================================================================
// 팝오버 아이콘 핸들러 - 탭2
// ============================================================================
void CShopSetupDlg::OnBnClickedAlarmSizeInfo()
{
    if (m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc; m_btnAlarmSizeInfo.GetWindowRect(&rc);
    m_popover.ShowAt(rc,
        _T("알림창 크기"),
        _T("알림창의 표시 크기를 설정합니다.\n매우 작게로 설정하면 화면 공간을 최소화합니다."),
        this);
}
void CShopSetupDlg::DrawInputBorders(CDC* /*pDC*/) {}
void CShopSetupDlg::DrawOneInputBorder(int /*ctrlId*/) {}
void CShopSetupDlg::DrawOneInputBorder(CDC* /*pDC*/, int /*ctrlId*/) {}

// ============================================================================
// OnDrawItem / OnMeasureItem
// owner-draw 컨트롤(CModernButton, CModernCheckBox 등)의 그리기 메시지를
// 컨트롤 자신에게 반사(Reflect)합니다.
// ============================================================================
void CShopSetupDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    // owner-draw 컨트롤이 반사된 WM_DRAWITEM을 직접 처리하므로
    // 기본 구현(CDialog::OnDrawItem)을 호출합니다.
    CDialog::OnDrawItem(nIDCtl, lpDIS);
}

void CShopSetupDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
    CDialog::OnMeasureItem(nIDCtl, lpMIS);
}


// ============================================================================
// DrawSectionIcon (하위 호환 stub - 탭 UI에서는 미사용)
// ============================================================================
void CShopSetupDlg::DrawSectionIcon(CDC* /*pDC*/, const CRect& /*rcIcon*/,
                                    SECTION_ICON_TYPE /*iconType*/) {}

// ============================================================================
// Timer (입력 hover 추적 - 필요시 확장)
// ============================================================================
void CShopSetupDlg::OnTimer(UINT_PTR nIDEvent) { CDialog::OnTimer(nIDEvent); }
void CShopSetupDlg::UpdateInputHoverByCursor() {}

// ============================================================================
// v10.1 - Toggle dependent edit enable/disable
// ============================================================================

void CShopSetupDlg::UpdateToggleDependentEdits(BOOL bForceRedraw /*= TRUE*/)
{
    // Card detect (priority transaction)
    if (m_editCardDetectParam.GetSafeHwnd() && m_chkCardDetect.GetSafeHwnd())
    {
        const BOOL bEnable = m_chkCardDetect.IsToggled();
        m_editCardDetectParam.EnableWindow(bEnable);

        if (!bEnable && ::GetFocus() == m_editCardDetectParam.GetSafeHwnd())
            m_tabCtrl.SetFocus();

        if (bForceRedraw)
            m_editCardDetectParam.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE);
    }

    // Scanner use
    if (m_editScannerPort.GetSafeHwnd() && m_chkScannerUse.GetSafeHwnd())
    {
        const BOOL bEnable = m_chkScannerUse.IsToggled();
        m_editScannerPort.EnableWindow(bEnable);

        if (!bEnable && ::GetFocus() == m_editScannerPort.GetSafeHwnd())
            m_tabCtrl.SetFocus();

        if (bForceRedraw)
            m_editScannerPort.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE);
    }
}

void CShopSetupDlg::OnBnClickedCardDetectToggle()
{
    UpdateToggleDependentEdits(TRUE);
}

void CShopSetupDlg::OnBnClickedScannerUseToggle()
{
    UpdateToggleDependentEdits(TRUE);
}
