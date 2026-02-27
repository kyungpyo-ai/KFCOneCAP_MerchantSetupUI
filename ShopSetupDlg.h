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
    afx_msg void OnTcnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedVanServerInfo();
    afx_msg void OnBnClickedPortInfo();
    afx_msg void OnBnClickedTaxPercentInfo();
    afx_msg void OnBnClickedCommTypeInfo();
    afx_msg void OnBnClickedCashReceiptInfo();
    afx_msg void OnBnClickedCardTimeoutInfo();
    afx_msg void OnBnClickedInterlockInfo();
    afx_msg void OnBnClickedMultiVoiceInfo();
    afx_msg void OnBnClickedCardDetectInfo();
    afx_msg void OnBnClickedScannerUseInfo();
    afx_msg void OnBnClickedAutoResetInfo();
    afx_msg void OnBnClickedAutoRebootInfo();
    afx_msg void OnBnClickedAlarmGraphInfo();
    afx_msg void OnBnClickedAlarmDualInfo();
    afx_msg void OnBnClickedSignPadUseInfo();
    afx_msg void OnBnClickedSignPadPortInfo();
    afx_msg void OnBnClickedSignPadSpeedInfo();
    afx_msg void OnBnClickedAlarmSizeInfo();

    DECLARE_MESSAGE_MAP()

private:
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


    // Tab0 카드 영역
    CRect m_rcCardServer;
    CRect m_rcCardPayMethod;
    // Tab3 카드 영역
    CRect m_rcCardShopDown;
    void InitializeFonts();
    void InitializeControls();
    void ApplyLayout();
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
    void     UpdateInputHoverByCursor();
};
