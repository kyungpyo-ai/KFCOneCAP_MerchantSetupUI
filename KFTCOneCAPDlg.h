#pragma once
#include "common.h"

#include "resource.h"
#include "ModernUI.h"
#include <gdiplus.h>
#include <shellapi.h>

class CHomeCardButton : public CButton
{
public:
    CHomeCardButton();

    BOOL IsHover() const { return m_bHover; }
    BOOL IsPressed() const { return m_bPressed; }
    int GetHoverProgress() const { return m_nHoverProgress; }
    int GetPressProgress() const { return m_nPressProgress; }
    void ResetVisualState();
    void ForceFadeOut();

protected:
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnCaptureChanged(CWnd* pWnd);
    afx_msg void OnCancelMode();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    DECLARE_MESSAGE_MAP()

private:
    void StartTrackMouseLeave();
    void StartAnimTimer();
    void StopAnimTimerIfIdle();
    void StepAnimation();

    BOOL m_bHover;
    BOOL m_bPressed;
    BOOL m_bTracking;
    int  m_nHoverProgress;
    int  m_nPressProgress;
    BOOL m_bIgnoreMouse;

};

class CKFTCOneCAPDlg : public CDialog
{
public:
    enum { IDD = IDD_KFTCONECAP_DIALOG };

    CKFTCOneCAPDlg(CWnd* pParent = NULL);
    virtual ~CKFTCOneCAPDlg();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnReaderSetup();
    afx_msg void OnShopSetup();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnTrans();
    afx_msg void OnReceiptSetup();
    afx_msg void OnMinimize();
    afx_msg void OnExit();
    afx_msg void OnLogTransfer();
    afx_msg void OnClose();
    afx_msg LRESULT OnTrayNotify(WPARAM wParam, LPARAM lParam);
    afx_msg void OnTrayOpen();
    afx_msg void OnTrayReader();
    afx_msg void OnTrayShop();
    afx_msg BOOL OnNcActivate(BOOL bActive);
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    DECLARE_MESSAGE_MAP()

private:
    enum HomeCardType
    {
        CARD_READER = 0,
        CARD_SHOP,
        CARD_TRANS,
        CARD_RECEIPT
    };

    void EnsureFonts();
    int  SX(int px) const;
    void LayoutControls();
    void LoadLogoImage();
    CString GetLogoPath() const;

    void DrawBackground(CDC& dc);
    void DrawHeader(CDC& dc);
    void DrawFooterDivider(CDC& dc);
    void DrawHomeCard(LPDRAWITEMSTRUCT lpDIS, HomeCardType type);

    void DrawCardIcon(Gdiplus::Graphics& g, const CRect& rcIcon, HomeCardType type, int nHoverProgress, int nPressProgress);
    void DrawReaderIcon(Gdiplus::Graphics& g, const Gdiplus::RectF& rc, const Gdiplus::Color& clr);
    void DrawShopIcon(Gdiplus::Graphics& g, const Gdiplus::RectF& rc, const Gdiplus::Color& clr);
    void DrawTransIcon(Gdiplus::Graphics& g, const Gdiplus::RectF& rc, const Gdiplus::Color& clr);
    void DrawReceiptIcon(Gdiplus::Graphics& g, const Gdiplus::RectF& rc, const Gdiplus::Color& clr);
    CString GetCardTitle(HomeCardType type) const;
    CString GetCardDescription(HomeCardType type) const;

private:
    struct CardCache {
        CDC      dc;
        CBitmap  bmp;
        CBitmap* pOldBmp;
        CSize    size;
        CardCache() : pOldBmp(NULL), size(0, 0) {}
    };
    CardCache m_cardCache[4];

    CHomeCardButton m_btnReaderCard;
    CHomeCardButton m_btnShopCard;
    CHomeCardButton m_btnTransCard;
    CHomeCardButton m_btnReceiptCard;
    CModernButton  m_btnMinimize;
    CModernButton  m_btnExit;
    CModernButton m_btnLogTransfer;

    CFont m_fontTitle;
    CFont m_fontSubtitle;
    CFont m_fontCardTitle;
    CFont m_fontCardDesc;
    CFont m_fontFooter;


    // [추가] GDI+ 전용 캐시 폰트 포인터
    // 렌더링 시 매번 생성하지 않고 여기서 재사용하여 성능을 최적화합니다.
    Gdiplus::Font* m_pGdiFontTitle;
    Gdiplus::Font* m_pGdiFontDesc;
    Gdiplus::Font* m_pGdiFontHeader;
    Gdiplus::Font* m_pGdiFontSub;

    enum EPendingOpen { PENDING_NONE = 0, PENDING_SHOP, PENDING_READER, PENDING_TRANS, PENDING_RECEIPT };

    BOOL         m_bFontsReady;
    EPendingOpen m_ePendingOpen;
    CBrush m_brBackground;
    NOTIFYICONDATA m_nid;
    CTrayPopup m_trayPopup;

    Gdiplus::Bitmap* m_pLogoBitmap;
    int m_nFooterDividerY;
};

