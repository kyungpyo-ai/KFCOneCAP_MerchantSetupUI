// ModernUI.h -   ( )

// ModernUI.h  
#pragma once
#include <afxwin.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <vector>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#ifndef REAL
typedef float REAL;
#endif

//  Blue Palette 
#define BLUE_100   RGB(168, 208, 255)   // #A8D0FF
#define BLUE_200   RGB(117, 180, 255)   // #75B4FF
#define BLUE_300   RGB( 66, 152, 255)   // #4298FF
#define BLUE_400   RGB( 15, 124, 255)   // #0F7CFF
#define BLUE_500   RGB(  0, 100, 221)   // #0064DD
// Gray Palette
#define GRAY_50    RGB(249,250,251)
#define GRAY_100   RGB(243,244,246)
#define GRAY_200   RGB(229,231,235)
#define GRAY_300   RGB(209,213,219)
#define GRAY_500   RGB(107,114,128)
#define GRAY_800   RGB(31,41,55)
#define BLUE_600   RGB(  0,  76, 168)   // #004CA8
#define BLUE_700   RGB(  9,  63, 129)   // #093F81
#define BLUE_800   RGB(  6,  52, 109)   // #06346D
#define BLUE_900   RGB(  2,  37,  79)   // #02254F
#define BLUE_50    RGB(235, 244, 255)   // #EBF4FF (surface tint)

#define KFTC_PRIMARY         BLUE_500
#define KFTC_PRIMARY_HOVER   BLUE_400
#define KFTC_TEXT_DARK       BLUE_800
#define KFTC_TEXT_LIGHT      RGB( 74, 111, 165)
#define KFTC_BG_LIGHT        BLUE_50
#define KFTC_BORDER          RGB(214, 228, 247)   // blue-tinted border

// ========================================
// Input theme (Combo/Edit )
// ========================================
#define KFTC_INPUT_BORDER_N      GRAY_200
#define KFTC_INPUT_BORDER_H      GRAY_300
#define KFTC_INPUT_BORDER_F      BLUE_500
#define KFTC_INPUT_RADIUS        6                   //  (px)
#define KFTC_INPUT_THICK_N       1.0f                //  ет(px)
#define KFTC_INPUT_THICK_F       2.0f


// ========================================
// Reusable input theme (Edit/Combo )
//  -   KFTC_INPUT_* ея 
// ========================================
struct KFTCInputTheme
{
    COLORREF borderN;   // 
    COLORREF borderH;   // hover
    COLORREF borderF;   // focus/pressed
    int      radius;    // round radius(px)
    float    thickN;    // normal thickness(px)
    float    thickF;    // focus thickness(px) (normal + 1px)
    int      marginLR;  // Edit / padding(px)

    KFTCInputTheme()
        : borderN(KFTC_INPUT_BORDER_N),
        borderH(KFTC_INPUT_BORDER_H),
        borderF(KFTC_INPUT_BORDER_F),
        radius(KFTC_INPUT_RADIUS),
        thickN(KFTC_INPUT_THICK_N),
        thickF(KFTC_INPUT_THICK_F),
        marginLR(10)
    {}
};

namespace ModernUITheme
{
    //   (еь   )
    const KFTCInputTheme& GetInputTheme();
    void SetInputTheme(const KFTCInputTheme& t);
}

// GDI+ lifetime helper (shared across controls/dialogs)
namespace ModernUIGfx
{
    void EnsureGdiplusStartup();
    void ShutdownGdiplus();
    bool IsGdiplusStarted();
}

// DPI scaling helpers (per-monitor DPI aware)
//  - Use base 96-DPI pixel values in layout constants, and call Scale() at runtime.
namespace ModernUIDpi
{
    UINT GetDpiForHwnd(HWND hwnd);
    int  Scale(HWND hwnd, int px);
    float ScaleF(HWND hwnd, float px);
}


// ========================================
// CModernButton -   
// ========================================
class CModernButton : public CButton
{
public:
    CModernButton();
    virtual ~CModernButton();

    void SetColors(COLORREF normalBg, COLORREF hoverBg, COLORREF textColor);

    // Underlay (background behind control) to avoid rounded-corner halo
    void SetUnderlayColor(COLORREF underlayBg);
    void ClearUnderlayColor();

protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();

    afx_msg LRESULT OnMouseHover(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);


    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    DECLARE_MESSAGE_MAP()

private:
    const KFTCInputTheme& GetActiveInputTheme() const;
    void AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius);

    COLORREF m_clrNormalBg;
    COLORREF m_clrHoverBg;
    COLORREF m_clrText;
    BOOL m_bHover;
    BOOL m_bTracking;

    // ()  
    int  m_nTextPx;
    BOOL m_bNoWrapEllipsis;
    BOOL m_bPressed;

    // Underlay
    BOOL     m_bUseUnderlayBg;
    COLORREF m_clrUnderlayBg;
    CBrush   m_brUnderlay; // for WM_CTLCOLOR (text background)
    COLORREF m_clrBrushBg;  // cached brush color
};

// ========================================
// CModernCheckBox -  
// ========================================
class CModernCheckBox : public CButton
{
public:
    CModernCheckBox();
    virtual ~CModernCheckBox();

    void SetChecked(BOOL bChecked);
    BOOL IsChecked() const { return m_bChecked; }
    void SetCardBackground(COLORREF color) { m_clrCardBg = color; }
    // Underlay (background behind control) to avoid rounded-corner halo
    void SetUnderlayColor(COLORREF underlayBg);
    void ClearUnderlayColor();
    // ()    -    
    void SetTextSizePx(int px) { m_nTextPx = px; Invalidate(); }
    void SetNoWrapEllipsis(BOOL bEnable) { m_bNoWrapEllipsis = bEnable; Invalidate(); }



    // () (Windows  )  /     
    //  - (FALSE):  GDI+    (   )
    //  - TRUE: GDI(DrawText)   (  )
    void SetUseGdiText(BOOL bUse) { m_bUseGdiText = bUse; Invalidate(); }
    void SetGdiTextYOffset(int y) { m_nGdiTextYOffset = y; Invalidate(); }
protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg LRESULT OnMouseHover(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()

private:
    const KFTCInputTheme& GetActiveInputTheme() const;
    void AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius);

    BOOL m_bChecked;
    BOOL m_bHover;
    BOOL m_bTracking;

    // ()  
    int  m_nTextPx;
    BOOL m_bNoWrapEllipsis;
    BOOL m_bUseGdiText;      // TRUE GDI(DrawText)  
    int  m_nGdiTextYOffset;  // GDI    (px)
    BOOL m_bPressed;
    COLORREF m_clrCardBg;
    // Underlay
    BOOL     m_bUseUnderlayBg;
    COLORREF m_clrUnderlayBg;
    CBrush   m_brUnderlay; // for WM_CTLCOLOR (text background)
    COLORREF m_clrBrushBg;  // cached brush color
};

// ========================================
// CPortToggleButton -   
// ========================================
class CPortToggleButton : public CButton
{
public:
    CPortToggleButton();
    virtual ~CPortToggleButton();

    void SetToggled(BOOL bToggled);
    BOOL IsToggled() const { return m_bToggled; }

protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg LRESULT OnMouseHover(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()

private:
    const KFTCInputTheme& GetActiveInputTheme() const;
    void AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius);

    BOOL m_bToggled;
    BOOL m_bHover;
    BOOL m_bTracking;

    // ()  
    int  m_nTextPx;
    BOOL m_bNoWrapEllipsis;
};

// ========================================
// CModernToggleSwitch (settings-style switch)
//  - Text on left, switch on right (TOSS/Kakao-like)
// ========================================
class CModernToggleSwitch : public CButton
{
public:
    CModernToggleSwitch();
    virtual ~CModernToggleSwitch();

    void SetToggled(BOOL bToggled);
    BOOL IsToggled() const { return m_bToggled; }

    void SetTextSizePx(int px) { m_nTextPx = px; Invalidate(FALSE); }
    void SetNoWrapEllipsis(BOOL b) { m_bNoWrapEllipsis = b; Invalidate(FALSE); }
    void SetUnderlayColor(COLORREF clr) { m_bUseUnderlay = TRUE; m_clrUnderlay = clr; Invalidate(FALSE); }

protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg LRESULT OnMouseHover(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()

private:
    void AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius);

    BOOL m_bToggled;
    BOOL m_bHover;
    BOOL m_bTracking;

    int  m_nTextPx;
    BOOL m_bNoWrapEllipsis;

    BOOL m_bUseUnderlay;
    COLORREF m_clrUnderlay;
};

// ========================================
// CSkinnedComboBox (smooth owner-draw dropdownlist)
//  - Closed state and list items are owner-drawn (no native text/button jitter)
//  - Popup list gets a clean 1px border (subclassed ComboLBox)
// ========================================
class CSkinnedComboBox : public CComboBox
{
public:
    CSkinnedComboBox();
    virtual ~CSkinnedComboBox();

    void SetTextPx(int px) { m_nTextPx = px; Invalidate(FALSE); }

    // Underlay (background behind control) to avoid rounded-corner halo
    void SetUnderlayColor(COLORREF underlayBg);
    void ClearUnderlayColor();

    // ()  :    
    void UseGlobalInputTheme(BOOL bUse = TRUE) { m_bUseGlobalTheme = bUse; Invalidate(FALSE); }
    void SetInputTheme(const KFTCInputTheme& t) { m_localTheme = t; m_bHasLocalTheme = TRUE; Invalidate(FALSE); }
    void ClearLocalInputTheme() { m_bHasLocalTheme = FALSE; Invalidate(FALSE); }
    void SetDropButtonWidth(int w) { m_nDropW = w; Invalidate(FALSE); }
    void SetCornerRadius(int r) { m_nRadius = r; Invalidate(FALSE); }


protected:
    virtual void PreSubclassWindow();
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
    virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnCbnDropdown();
    afx_msg void OnCbnCloseup();
    afx_msg void OnCbnSelchange();
    afx_msg void OnCbnSelendok();
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnSize(UINT nType, int cx, int cy);

    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg LRESULT OnSetFontMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSetTextMsg(WPARAM wParam, LPARAM lParam);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnPaint();
    afx_msg LRESULT OnPrintClientMsg(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

private:
    void TrackMouseLeave();
    const KFTCInputTheme& GetActiveInputTheme() const;

    CString GetDisplayText() const;
    void AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius);

    void PaintComboToDC(CDC& dc);

    void HookListBox(HWND hList);
    void UnhookListBox();

    static LRESULT CALLBACK ListBoxProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    // state
    BOOL m_bHover;
    BOOL m_bTracking;
    BOOL m_bFocus;
    BOOL m_bDropped;
    int  m_nSelOnDrop;
    BOOL m_bSelCommitted;
    BOOL m_bHoverTimer;


    BOOL m_bInPaint;
    // popup list subclass
    HWND m_hList;
    WNDPROC m_oldListProc;

    // cached font for GDI+ text (avoid per-paint CreateFontIndirect)
    HFONT   m_hTextFontCache;
    LOGFONT m_lfTextCache;
    BOOL    m_bHasTextFontCache;

    // theme
    BOOL m_bUseGlobalTheme;
    BOOL m_bHasLocalTheme;
    KFTCInputTheme m_localTheme;

    // tunables
    int m_nTextPx;
    int m_nDropW;
    int m_nRadius;

    // Underlay
    BOOL     m_bUseUnderlayBg;
    COLORREF m_clrUnderlayBg;
    CBrush   m_brUnderlay; // for WM_CTLCOLOR (text background)
    COLORREF m_clrBrushBg;  // cached brush color
};

// ========================================
// CSkinnedEdit -    еш(Edit)
//  - еш//ии ет Combo  
// ========================================
class CSkinnedEdit : public CEdit
{
public:
    CSkinnedEdit();
    virtual ~CSkinnedEdit();

    void SetRadius(int r) { m_nRadius = r; Invalidate(FALSE); }
    void SetTextPx(int px) { m_nTextPx = px; Invalidate(FALSE); }

    // Underlay (background behind control) to avoid rounded-corner halo
    void SetUnderlayColor(COLORREF underlayBg);
    void ClearUnderlayColor();

protected:
    virtual void PreSubclassWindow();

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg LRESULT OnSetFontMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSetTextMsg(WPARAM wParam, LPARAM lParam);

    afx_msg LRESULT OnPrintClientMsg(WPARAM wParam, LPARAM lParam);

    afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);

    DECLARE_MESSAGE_MAP()

private:
    const KFTCInputTheme& GetActiveInputTheme() const;
    void AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius);
    void ApplyThemeAndMargins();
    void EnsureMultilineForVCenter();
    void TrackMouseLeave();

    // theme
    BOOL m_bUseGlobalTheme;
    BOOL m_bHasLocalTheme;
    KFTCInputTheme m_localTheme;

    BOOL m_bHover;
    BOOL m_bTracking;
    BOOL m_bFocus;
    BOOL m_bInPaint;

    int  m_nRadius;
    int  m_nTextPx;

    // Underlay
    BOOL     m_bUseUnderlayBg;
    COLORREF m_clrUnderlayBg;
    CBrush   m_brUnderlay; // for WM_CTLCOLOR (text background)
    COLORREF m_clrBrushBg;  // cached brush color
};

// ============================================================
// CModernTabCtrl - ии  
// ============================================================
struct ModernTabItem
{
    CStringW text;
    int      iconType; // 0=server 1=device 2=system 3=download
};



// ========================================
// Info Text (Read-only value display)
//  - (Edit)  , /    
//  -  Row/Section        
// ========================================
class CInfoText : public CStatic
{
public:
    CInfoText() : m_crUnderlay(::GetSysColor(COLOR_3DFACE)), m_crText(RGB(55, 65, 81)),
        m_crPlaceholder(RGB(156, 163, 175)),
        m_nPadX(8), m_nPadY(0), m_bBold(FALSE) {}

    void SetUnderlayColor(COLORREF cr) { m_crUnderlay = cr; Invalidate(FALSE); }
    void SetTextColor(COLORREF cr) { m_crText = cr;     Invalidate(FALSE); }
    void SetPadding(int padX, int padY = 0) { m_nPadX = padX; m_nPadY = padY; Invalidate(FALSE); }
    void SetBold(BOOL bBold) { m_bBold = bBold; Invalidate(FALSE); }

    // text     (placeholder)
    void SetPlaceholder(LPCTSTR s) { m_strPlaceholder = s; Invalidate(FALSE); }
    void SetPlaceholderColor(COLORREF cr) { m_crPlaceholder = cr; Invalidate(FALSE); }

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    DECLARE_MESSAGE_MAP()

private:
    COLORREF m_crUnderlay;
    COLORREF m_crText;
    COLORREF m_crPlaceholder;
    CString  m_strPlaceholder;
    int      m_nPadX;
    int      m_nPadY;
    BOOL     m_bBold;
};
class CModernTabCtrl : public CWnd
{
public:
    CModernTabCtrl();
    virtual ~CModernTabCtrl();

    BOOL Create(CWnd* pParent, UINT nID, const CRect& rect);

    void AddTab(LPCTSTR text, int iconType);
    int  GetCurSel() const { return m_nSel; }
    void SetCurSel(int n);
    int  GetCount()  const { return (int)m_items.size(); }

    //    (px) - еш   ЕZ 
    static const int kBarH = 44;

protected:
    afx_msg void    OnPaint();
    afx_msg BOOL    OnEraseBkgnd(CDC* pDC);
    afx_msg void    OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void    OnMouseMove(UINT nFlags, CPoint pt);
    afx_msg void    OnMouseLeave();
    afx_msg LRESULT OnMouseHover(WPARAM wp, LPARAM lp);
    DECLARE_MESSAGE_MAP()

private:
    std::vector<ModernTabItem> m_items;
    int  m_nSel;
    int  m_nHover;
    bool m_bTrack;

    CFont  m_font;
    CFont  m_fontBold;
    CBrush m_brushBg;
    Gdiplus::FontFamily* m_pTabFontFamily;
    Gdiplus::Font*       m_pTabFontN;
    Gdiplus::Font*       m_pTabFontB;

    RectF GetTabRect(int idx) const;
    void DrawTab(Graphics& g, int idx, const RectF& rc);
    void DrawIcon(Graphics& g, int iconType,
        REAL cx, REAL cy,
        REAL sz, bool active);
    void EnsureTrack();
};
// ========================================
// CInfoIconButton - small circular "i" info icon button
// ========================================
class CInfoIconButton : public CButton
{
public:
    CInfoIconButton();
    virtual ~CInfoIconButton() {}

    void SetUnderlayColor(COLORREF clr) { m_bUseUnderlay = TRUE; m_clrUnderlay = clr; }

protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
        afx_msg BOOL OnEraseBkgnd(CDC* pDC);
DECLARE_MESSAGE_MAP()

private:
    BOOL     m_bHover;
    BOOL     m_bTracking;
    BOOL     m_bUseUnderlay;
    COLORREF m_clrUnderlay;
};

// ========================================
// CModernPopover - floating info popover (Toss/Kakao style)
// ========================================
class CModernPopover : public CWnd
{
public:
    CModernPopover();
    virtual ~CModernPopover() {}

    void ShowAt(const CRect& anchorScreenRect, LPCTSTR title, LPCTSTR body, CWnd* pParent);
    void Hide();
    BOOL IsVisible() const { return m_bVisible; }

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()

private:
    static void RegisterPopoverClass();
    void AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& r, REAL radius);
        void AddPopoverPath(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& cardRc, REAL radius, float arrowX, float arrowTipY, float arrowHalfW);
void RefreshLayered();
    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    CString m_strTitle;
    CString m_strBody;
    int     m_nArrowX;
    int     m_nCardW;   // DPI-scaled card width (without shadow padding)
    int     m_nCardH;   // DPI-scaled card height (auto-sized from text)
    BOOL    m_bVisible;
    static HHOOK           s_hMouseHook;
    static CModernPopover* s_pPopoverInst;

    // Base sizes are in 96-DPI pixels. Actual window size is DPI-scaled.
// Popover height is auto-sized from text (minimum kPopMinH).
    static const int kPopW = 292; // toss-like width
    static const int kPopMinH = 96;
    static const int kArrowH = 10;
    static const int kShadowPad = 16;
    static const int kShadowOffY = 8;
};
