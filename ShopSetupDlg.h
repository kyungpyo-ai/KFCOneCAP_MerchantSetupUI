// ShopSetupDlg.h - 탭 UI 버전

#pragma once

#ifndef IDC_BTN_VAN_SERVER_INFO
#define IDC_BTN_VAN_SERVER_INFO      60100
#endif
#ifndef IDC_BTN_PORT_INFO
#define IDC_BTN_PORT_INFO            60101
#endif
#ifndef IDC_BTN_COMM_TYPE_INFO
#define IDC_BTN_COMM_TYPE_INFO       60102
#endif
#ifndef IDC_BTN_CASH_RECEIPT_INFO
#define IDC_BTN_CASH_RECEIPT_INFO    60103
#endif
#ifndef IDC_BTN_CARD_TIMEOUT_INFO
#define IDC_BTN_CARD_TIMEOUT_INFO    60104
#endif
#ifndef IDC_BTN_INTERLOCK_INFO
#define IDC_BTN_INTERLOCK_INFO       60105
#endif
#ifndef IDC_BTN_MULTI_VOICE_INFO
#define IDC_BTN_MULTI_VOICE_INFO     60109
#endif
#ifndef IDC_BTN_CARD_DETECT_INFO
#define IDC_BTN_CARD_DETECT_INFO     60110
#endif
#ifndef IDC_BTN_SCANNER_USE_INFO
#define IDC_BTN_SCANNER_USE_INFO     60111
#endif
#ifndef IDC_BTN_AUTO_RESET_INFO
#define IDC_BTN_AUTO_RESET_INFO     60112
#endif
#ifndef IDC_BTN_AUTO_REBOOT_INFO
#define IDC_BTN_AUTO_REBOOT_INFO     60113
#endif
#ifndef IDC_BTN_ALARM_GRAPH_INFO
#define IDC_BTN_ALARM_GRAPH_INFO     60114
#endif
#ifndef IDC_BTN_ALARM_DUAL_INFO
#define IDC_BTN_ALARM_DUAL_INFO     60115
#endif
#ifndef IDC_BTN_TAX_PERCENT_INFO
#define IDC_BTN_TAX_PERCENT_INFO     60116
#endif
#ifndef IDC_BTN_SIGN_PAD_PORT_INFO
#define IDC_BTN_SIGN_PAD_PORT_INFO   60117
#endif

#ifndef IDC_BTN_SIGN_PAD_USE_INFO
#define IDC_BTN_SIGN_PAD_USE_INFO    60106
#endif
#ifndef IDC_BTN_SIGN_PAD_SPEED_INFO
#define IDC_BTN_SIGN_PAD_SPEED_INFO  60107
#endif
#ifndef IDC_BTN_ALARM_SIZE_INFO
#define IDC_BTN_ALARM_SIZE_INFO      60108
#endif
#include "ModernUI.h"
#include "resource.h"
#include "ShopDownDlg.h"

#ifndef IDC_STATIC_RECT
#define IDC_STATIC_RECT 60001
#endif
#ifndef IDC_TAB_MAIN
#define IDC_TAB_MAIN    60002
#endif

#ifndef IDC_STATIC_SCANNER_PORT_LABEL
#define IDC_STATIC_SCANNER_PORT_LABEL 60003
#endif

#ifndef IDC_STATIC_ERR_PORT
#define IDC_STATIC_ERR_PORT 60020
#endif
#ifndef IDC_STATIC_ERR_NO_SIGN
#define IDC_STATIC_ERR_NO_SIGN 60021
#endif
#ifndef IDC_STATIC_ERR_TAX
#define IDC_STATIC_ERR_TAX 60022
#endif
#ifndef IDC_STATIC_ERR_CARD_DETECT_PROGRAM
#define IDC_STATIC_ERR_CARD_DETECT_PROGRAM 60023
#endif
#ifndef IDC_STATIC_ERR_TIMEOUT
#define IDC_STATIC_ERR_TIMEOUT 60024
#endif
#ifndef IDC_STATIC_ERR_SIGNPAD_PORT
#define IDC_STATIC_ERR_SIGNPAD_PORT 60025
#endif
#ifndef IDC_STATIC_ERR_SCANNER_PORT
#define IDC_STATIC_ERR_SCANNER_PORT 60026
#endif

// ==============================================================
// [ShopSetupDlg.h]
//  - 메인 설정 화면(결제 설정/장치 정보/시스템 설정 등)을 담당
//  - ModernUI 컨트롤을 생성/배치하고, 레지스트리(프로파일)와 값을 동기화
//
// 구현 포인트
//  - OnInitDialog()에서 컨트롤 생성 → 레이아웃 적용 → 레지스트리 로드
//  - OK/적용 시 SaveOptionsToRegistry()로 현재 UI 값을 저장
//  - 라벨 옆 'i' 아이콘은 CInfoIconButton + CModernPopover로 구현
// ==============================================================


class CShopSetupDlg : public CDialog
{
    DECLARE_DYNAMIC(CShopSetupDlg)

public:
    CShopSetupDlg(CWnd* pParent = nullptr);
    virtual ~CShopSetupDlg();

    enum { IDD = IDD_SHOP_SETUP_DLG };

    // 서버 연결
    CSkinnedComboBox m_comboVanServer;
    CSkinnedEdit    m_editPort;
    CSkinnedEdit    m_editNoSignAmount;
    CSkinnedEdit    m_editTaxPercent;
    CSkinnedEdit    m_editCardTimeout;
    CSkinnedEdit    m_editCardDetectParam;
    CSkinnedEdit    m_editSignPadPort;
    CSkinnedEdit    m_editScannerPort;

    int m_intPort;

    // 카드/결제
    int m_intCardTimeout;
    int m_intNoSignAmount;
    int m_intTaxPercent;
    CSkinnedComboBox m_comboCashReceipt;
    CSkinnedComboBox m_comboInterlock;    CSkinnedComboBox m_comboCommType;
    CModernToggleSwitch m_chkCardDetect;
    CString m_strCardDetectParam;
    CModernToggleSwitch m_chkMultiVoice;

    // 서명패드
    CSkinnedComboBox m_comboSignPadUse;
    int m_intSignPadPort;
    CSkinnedComboBox m_comboSignPadSpeed;

    // 기타 장치
    CModernToggleSwitch m_chkScannerUse;
    int m_intScannerPort;

    // 알람창 설정
    CSkinnedComboBox m_comboAlarmPos;
    CSkinnedComboBox m_comboAlarmSize;
    CModernToggleSwitch m_chkAlarmGraph;
    CModernToggleSwitch m_chkAlarmDual;

    // 시스템 동작
    CModernToggleSwitch m_chkAutoReset;
    CModernToggleSwitch m_chkAutoReboot;

    // 단축키
    CSkinnedComboBox m_comboCancelKey;
    CSkinnedComboBox m_comboMSRKey;

    // ShopDownDlg
    CStatic m_staticShopContainer;

    // Popover info buttons
    CInfoIconButton m_btnVanInfo;           // Tab0: 금융결제원 서버
    CInfoIconButton m_btnPortInfo;          // Tab0: 포트번호
    CInfoIconButton m_btnTaxPercentInfo;   // Tab0: 세금 자동 역산
    CInfoIconButton m_btnCommTypeInfo;      // Tab0: 통신방식
    CInfoIconButton m_btnCashReceiptInfo;   // Tab0: 현금영수증 거래
    CInfoIconButton m_btnCardTimeoutInfo;   // Tab1: 카드입력 Timeout
    CInfoIconButton m_btnInterlockInfo;     // Tab1: 장치 연동 방식
    CInfoIconButton m_btnMultiVoiceInfo;    // Tab1: 멀티패드 음성 출력
    CInfoIconButton m_btnCardDetectInfo;   // Tab0: 우선 거래
    CInfoIconButton m_btnScannerUseInfo;   // Tab1: 스캐너 사용
    CInfoIconButton m_btnAutoResetInfo;    // Tab2: 자동 재실행
    CInfoIconButton m_btnAutoRebootInfo;   // Tab2: 자동 리부팅
    CInfoIconButton m_btnAlarmGraphInfo;   // Tab2: 알림창 그림
    CInfoIconButton m_btnAlarmDualInfo;    // Tab2: 알림창 듀얼
    CInfoIconButton m_btnSignPadUseInfo;    // Tab1: 서명패드 사용
    CInfoIconButton m_btnSignPadPortInfo;   // Tab1: 서명패드 포트번호
    CInfoIconButton m_btnSignPadSpeedInfo;  // Tab1: 서명패드 속도
    CInfoIconButton m_btnAlarmSizeInfo;     // Tab2: 알림창 크기
    CModernPopover  m_popover;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
    afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDestroy();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL OnNcActivate(BOOL bActive);   // [FIX v2.1] DefDlgProc xxxSaveDlgFocus O(N^2) 차단
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnTcnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnInfoButtonClicked(UINT nID); // unified handler for all 18 info buttons (IDC_BTN_VAN_SERVER_INFO..IDC_BTN_SIGN_PAD_PORT_INFO)

    // Unified popover helper
    void ShowInfoPopover(CInfoIconButton& btn, LPCTSTR szTitle, LPCTSTR szBody);

    // v10.1: toggle-dependent edits
    afx_msg void OnCbnSelchangeSignPadUse();
    afx_msg void OnBnClickedCardDetectToggle();
    afx_msg void OnBnClickedScannerUseToggle();
    afx_msg void OnEnChangeValidateInput();

    DECLARE_MESSAGE_MAP()

private:
    enum ValidationField
    {
        VF_NONE = 0,
        VF_PORT,
        VF_NO_SIGN_AMOUNT,
        VF_TAX_PERCENT,
        VF_CARD_DETECT_PROGRAM,
        VF_CARD_TIMEOUT,
        VF_SIGNPAD_PORT,
        VF_SCANNER_PORT
    };

    struct ValidationBinding
    {
        int ctrlId;
        int errId;
        ValidationField field;
        int tabIndex;
    };

    // -----------------------------------------------------------------
    // 검증 헬퍼 함수
    // - ValidationBinding 테이블을 기준으로 "어떤 Edit / 어떤 오류문구 / 어느 탭"인지 연결한다.
    // - 실시간 입력(EN_CHANGE)에서는 ValidateControlAndUpdateUI()로 현재 Edit 1개만 검사한다.
    // - 확인 버튼에서는 ValidateAllInputs()로 전체 항목을 순회하고 첫 오류 Edit를 찾아낸다.
    // - SetValidationText()/SetEditValidationErrorState()는 오류문구와 빨간 테두리 UI만 갱신한다.
    // - RefreshValidationVisibilityByTab()는 현재 탭에 속한 오류문구만 보이게 정리한다.
    // -----------------------------------------------------------------
    void EnsureValidationStatics();
    void SetValidationText(int nStaticId, const CString& text);
    void SetEditValidationErrorState(int nCtrlId, BOOL bHasError);
    CSkinnedEdit* GetSkinnedEditByCtrlId(int nCtrlId);
    void PositionValidationText(int nStaticId, int x, int y, int w, int h, BOOL bShow = TRUE);
    // 항목 1개에 대한 실제 검증 규칙(숫자/범위/필수입력/조건부검증)을 수행한다.
    BOOL ValidateSingleField(ValidationField field, CString& outMessage) const;
    // 전체 입력값을 검사한다. bUpdateUI=TRUE면 오류문구/테두리까지 같이 갱신한다.
    BOOL ValidateAllInputs(BOOL bUpdateUI, int* pFirstInvalidCtrlId = NULL);
    // 특정 Edit 1개만 검사하고, 해당 오류문구/오류테두리 UI를 최소 범위로 반영한다.
    void ValidateControlAndUpdateUI(int nCtrlId);
    const ValidationBinding* FindValidationBinding(int nCtrlId) const;
    const ValidationBinding* FindValidationBindingByErrId(int nStaticId) const;
    int GetTabIndexForControl(int nCtrlId) const;
    // 탭 전환 후 현재 활성 탭에 속한 오류문구만 보이도록 visibility를 재정리한다.
    void RefreshValidationVisibilityByTab();
    static const ValidationBinding* GetValidationBindings(int& outCount);
    static BOOL IsDigitsOnly(const CString& text);
    static BOOL IsPositiveNumberText(const CString& text);

private:
    // 토글/콤보 상태에 따라 종속 Edit의 Enable/Disable을 바꾸고,
    // 비활성화되는 항목은 오류문구/오류테두리를 즉시 제거한다.
    // 예) 카드 감지 우선 거래 OFF -> 우선 거래 프로그램 검증 제외
    //     스캐너 사용 OFF           -> 스캐너 포트번호 검증 제외
    //     서명패드 사용 아님        -> 서명패드 포트번호 검증 제외
    void UpdateToggleDependentEdits(BOOL bForceRedraw = TRUE);

    struct SettingsSnapshot {
        int    intPort, intCardTimeout, intNoSignAmount, intTaxPercent;
        int    intSignPadPort, intScannerPort;
        CString strCardDetectParam;
        int    cmbVanServer, cmbCashReceipt, cmbInterlock, cmbCommType;
        int    cmbSignPadUse, cmbSignPadSpeed;
        int    cmbAlarmPos, cmbAlarmSize, cmbCancelKey, cmbMSRKey;
        BOOL   tglCardDetect, tglMultiVoice, tglScannerUse;
        BOOL   tglAlarmGraph, tglAlarmDual, tglAutoReset, tglAutoReboot;
    };
    SettingsSnapshot m_snap;
    void TakeSnapshot();
    BOOL HasChanges() const;
    // 레지스트리 값을 읽어 UI에 반영한다. 값이 없으면 기본값을 사용한다.
    void LoadOptionsFromRegistry();
    // 현재 UI 값을 레지스트리에 저장한다.
    void SaveOptionsToRegistry();

    enum SECTION_ICON_TYPE
    {
        ICON_SQUARE   = 0,
        ICON_TRIANGLE = 1,
        ICON_SERVER   = 10,
        ICON_DEVICE   = 11,
        ICON_SYSTEM   = 12,
        ICON_DOWNLOAD = 13,
    };

    CBrush m_brushBg;
    CBrush m_brushWhite;
    CBrush m_brushSection;
    CBrush m_brushTabContent;

    CFont m_fontTitle;
    CFont m_fontSubtitle;
    CFont m_fontSection;
    CFont m_fontLabel;
    CFont m_fontGroupTitle;
    CFont m_fontGroupBracket;
    CFont m_fontValidation;

    CModernButton m_btnOk;
    CModernButton m_btnCancel;

    // 탭 컨트롤
    CModernTabCtrl m_tabCtrl;
    int         m_nActiveTab;

    CShopDownDlg m_shopDownDlg;
    CStatic      m_staticRectHost;

    // 그룹 소제목 영역 (OnPaint에서 사용)
    CRect m_rcGrpPay;
    CRect m_rcGrpReader;
    CRect m_rcGrpSign;
    CRect m_rcGrpEtc;
    CRect m_rcGrpAlarm;
    CRect m_rcGrpSystem;
    CRect m_rcGrpHotkey;

    // 탭 내부 컨텐츠 영역 (탭 컨트롤 아래 부분)
    CRect m_rcTabContent;
    CRect m_rcOuterCard;


    // Tab0 카드 영역
    CRect m_rcCardServer;
    CRect m_rcCardPayMethod;
    // Tab3 카드 영역
    CRect m_rcCardShopDown;
    void EnsureFonts();
    void InitializeControls();
    void ApplyLayout();

    // --- Layout helpers (called from ApplyLayout) ---
    int  SX(int px) const;
    void MoveCtrl(int nID, int x, int y, int w, int h, BOOL bShow = FALSE);
    void ApplyLayoutTab0();  // Tab 0: card reader settings
    void ApplyLayoutTab1();  // Tab 1+2: devices and system settings
    void ApplyLayoutTab3();  // Tab 3: merchant download
    int  CalculateRequiredHeight();
    void ShowTab(int nTab);

    void DrawBackground(CDC* pDC);
    void DrawGroupLabels(CDC* pDC);
    void DrawSectionIcon(CDC* pDC, const CRect& rcIcon, SECTION_ICON_TYPE iconType);

    void DrawInputBorders();
    void DrawInputBorders(CDC* pDC);
    void DrawOneInputBorder(int ctrlId);
    void DrawOneInputBorder(CDC* pDC, int ctrlId);

    enum { TIMER_INPUT_HOVER_TRACK = 0x4A21 };
    UINT_PTR m_uHoverTimer;
    int      m_nHoverInputId;
    BOOL     m_bUiInitialized;
    BOOL     m_bClosing;
    void     UpdateInputHoverByCursor();

    HFONT m_hFontCardTitle;   // section card subtitle (bold, DPI-scaled 13px)
    HFONT m_hFontHdrTitle;    // header title (bold, DPI-scaled 16px)
    HFONT m_hFontHdrSub;      // header subtitle (regular, DPI-scaled 11px)
};