// ModernUI.h -   ( )

// ModernUI.h  
#pragma once
#include <afxwin.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#ifndef REAL
typedef float REAL;
#endif

// Gray Palette
#define GRAY_50    RGB(249,250,251)
#define GRAY_100   RGB(243,244,246)
#define GRAY_200   RGB(229,231,235)
#define GRAY_300   RGB(209,213,219)
#define GRAY_500   RGB(107,114,128)
#define GRAY_800   RGB(31,41,55)

//  Blue Palette 
#define BLUE_100   RGB(168, 208, 255)   // #A8D0FF
#define BLUE_200   RGB(117, 180, 255)   // #75B4FF
#define BLUE_300   RGB( 66, 152, 255)   // #4298FF
#define BLUE_400   RGB( 15, 124, 255)   // #0F7CFF
#define BLUE_500   RGB(  0, 100, 221)   // #0064DD
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
#define KFTC_DISABLED_BG     GRAY_100
#define KFTC_DISABLED_BORDER GRAY_200
#define KFTC_DISABLED_TEXT   GRAY_500
#define KFTC_DISABLED_ICON   GRAY_300
#define KFTC_BTN_SECONDARY     RGB(235, 236, 240)   // secondary (Delete) btn normal
#define KFTC_BTN_SECONDARY_HOV RGB(225, 226, 230)   // secondary (Delete) btn hover
#define KFTC_BTN_DISABLED_BG   RGB(235, 237, 240)   // primary btn disabled bg
#define KFTC_CARD_PENDING_BG   RGB(246, 248, 251)   // row card bg when no data
#define KFTC_DLG_CONTENT_BG    RGB(250, 251, 253)   // ShopDownDlg content bg
#define KFTC_CARD_DISABLED_BG  RGB(245, 246, 248)   // disabled control surface

// Text hierarchy (info panels, card layouts)
#define KFTC_INFO_LABEL        RGB(120, 128, 142)    // secondary label text
#define KFTC_INFO_VALUE        RGB( 24,  28,  35)    // primary value text
#define KFTC_INFO_EMPTY        RGB(160, 165, 175)    // placeholder / empty text
#define KFTC_EDIT_TEXT         RGB( 18,  24,  40)    // edit control text (enabled)
#define KFTC_EDIT_TEXT_DIS     RGB(120, 125, 135)    // edit control text (disabled)

// Home screen (KFTCOneCAPDlg)
#define KFTC_HOME_BG           RGB(248, 250, 252)    // home dialog background
#define KFTC_CARD_BG           RGB(255, 255, 255)    // home card background
#define KFTC_CARD_BORDER       RGB(232, 236, 242)    // home card border (normal)
#define KFTC_CARD_BORDER_H     RGB(226, 232, 240)    // home card border (hover)
#define KFTC_CARD_FILL_PRE     RGB(210, 230, 255)    // home card fill (pressed)
#define KFTC_TITLE_TEXT        RGB( 25,  31,  40)    // page title text
#define KFTC_CARD_TITLE_T      RGB( 51,  61,  75)    // card title text
#define KFTC_SUB_TEXT          RGB(139, 149, 161)    // subtitle / description text
#define KFTC_FOOTER_DIV        RGB(220, 226, 234)    // footer divider line

// Table / integrity check (ReaderSetupDlg)
#define KFTC_TBL_BORDER        RGB(214, 223, 235)    // table outer border
#define KFTC_TBL_HDR_BG        RGB(250, 251, 253)    // table header background
#define KFTC_TBL_HDR_LINE      RGB(238, 241, 247)    // table header separator
#define KFTC_TBL_ROW_LINE      RGB(242, 245, 249)    // table row divider
#define KFTC_TBL_ALT_ROW       RGB(251, 252, 255)    // table alternate row background
#define KFTC_TBL_HDR_TEXT      RGB( 26,  32,  44)    // table header text
#define KFTC_TBL_BODY_TXT      RGB( 92, 104, 122)    // table body text
#define KFTC_TBL_EMPTY_TXT     RGB(160, 168, 180)    // table empty state text
#define KFTC_TBL_SCRL_TRK      RGB(243, 246, 250)    // scroll track
#define KFTC_TBL_SCRL_THB      RGB(196, 205, 216)    // scroll thumb


// ========================================
// Input theme (Combo/Edit )
// ========================================
#define KFTC_INPUT_BORDER_N      GRAY_200
#define KFTC_INPUT_BORDER_H      GRAY_300
#define KFTC_INPUT_BORDER_F      BLUE_500
#define KFTC_INPUT_RADIUS        6                   //  (px)
#define KFTC_INPUT_THICK_N       1.0f                //  β(px)
#define KFTC_INPUT_THICK_F       2.0f

// ==============================================================
// [ModernUI.h]
//  - 현대화 UI(네이버/토스/카카오 스타일) 구현을 위한 공용 헤더
//
// 포함 내용
//  1) 팔레트/상수(색상, 라운드, 두께)
//  2) Theme/스킨 인터페이스(ModernUITheme)
//  3) GDI+ / DPI 유틸(ModernUIGfx, ModernUIDpi)
//  4) 커스텀 컨트롤들
//     - CModernButton / CModernCheckBox / CModernToggleSwitch
//     - CSkinnedComboBox / CSkinnedEdit (인풋 보더/포커스 강조)
//     - CModernTabCtrl (탭 버튼 스타일)
//     - CInfoIconButton / CModernPopover (라벨 옆 도움말 팝오버)
//
// 유지보수 팁
//  - 색/두께/라운드가 바뀌면 우선 KFTC_* / ModernUITheme 설정부터 점검
//  - 컨트롤 개별 동작(hover/focus/pressed)은 각 클래스의 메시지 처리에서 관리
// ==============================================================



// ========================================
// Reusable input theme (Edit/Combo )
//  -   KFTC_INPUT_* ο 
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
    //   (μ   )
    const KFTCInputTheme& GetInputTheme();
    void SetInputTheme(const KFTCInputTheme& t);
}

// GDI+ lifetime helper (shared across controls/dialogs)
namespace ModernUIGfx
{
    void EnsureGdiplusStartup();
    void ShutdownGdiplus();
    bool IsGdiplusStarted();
    // Shared rounded-rect path builder (use instead of per-class lambdas)
    void AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& r, float radius);
}

// Shared modern UI font loader/helper (Pretendard + fallback)
namespace ModernUIFont
{
    bool EnsureFontsLoaded();
    void ShutdownFonts();
    LPCTSTR GetUIFontFace();
    void ApplyUIFontFace(LOGFONT& lf);
    Gdiplus::FontFamily* CreateGdipFontFamily();
    Gdiplus::Font* CreateGdipFontFromLogFont(const LOGFONT& lf);
}

// Windows 기본 제목표시줄을 최대한 밝은 흰색 톤으로 맞춘다.
// - Windows 11에서는 캡션/글자/테두리 색 지정이 비교적 잘 반영된다.
// - Windows 10 등 미지원 환경에서는 실패해도 조용히 무시한다.
namespace ModernUIWindow
{
    void ApplyWhiteTitleBar(HWND hwnd);
}

// Standard dialog header drawing helper (icon + title/subtitle + divider)
// Both CShopSetupDlg and CReaderSetupDlg use this to keep header rendering identical.
namespace ModernUIHeader
{
    enum class IconType { CardTerminal, Store, Transaction, Receipt };
    // bx, by, bsz : icon rect (already DPI-scaled pixels)
    // hFontTitle/hFontSub : GDI HFONT handles (may be NULL to skip)
    // divX1, divY, divX2 : divider line coords (pass divY=0 to skip)
    void Draw(HDC hdc,
              float bx, float by, float bsz, IconType iconType,
              LPCWSTR wTitle, LPCWSTR wSub,
              HFONT hFontTitle, HFONT hFontSub,
              int divX1, int divY, int divX2, float subOffsetY = 26.0f, float badgeOffsetY = 0.0f);
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
// ============================================================
// ButtonStyle: CModernButton 시각 스타일 열거형.
// SetButtonStyle()을 OnInitDialog에서 호출하면 텍스트 기반 자동 감지보다 우선 적용된다.
// 기본값(Auto)은 기존 텍스트 감지 동작을 그대로 유지한다.
// ============================================================
enum class ButtonStyle {
    Auto,       // 기본값: 버튼 텍스트로 스타일 자동 감지 (하위 호환)
    Default,    // 흰 배경 + 테두리
    Primary,    // 파랑 채움 (확인/제출)
    Danger,     // 빨강 (삭제/위험 동작)
    Download,   // 연파랑 (다운로드)
    Reader,     // Reader button (init/update/status)
    Exit        // 회색 테두리 (취소/닫기)
};
class CModernButton : public CButton
{
public:
    CModernButton();
    virtual ~CModernButton();

    void SetColors(COLORREF normalBg, COLORREF hoverBg, COLORREF textColor);
    void SetHoverTextColor(COLORREF hoverTextColor);   // hover 시 텍스트 색상 변경 지원

    // Underlay (background behind control) to avoid rounded-corner halo
    void SetUnderlayColor(COLORREF underlayBg);
    void ClearUnderlayColor();
    void SetButtonStyle(ButtonStyle style);
    void SetLoading(BOOL bLoading, LPCTSTR lpszLoadingText = NULL);
    void DrawLoadingFrame();  // Draw animation frame directly to DC (no Invalidate/WM_PAINT)
    BOOL IsLoading() const { return m_bLoading; }
    void SetTextPx(int px) { m_nTextPx = px; Invalidate(FALSE); }
    CString GetBaseText() const { return m_strBaseText; } // 버튼 스타일 명시 설정

protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();

    afx_msg LRESULT OnMouseHover(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    DECLARE_MESSAGE_MAP()

private:
    ButtonStyle m_style; // 버튼 시각 스타일 (Auto이면 텍스트로 자동 감지)

    COLORREF m_clrNormalBg;
    COLORREF m_clrHoverBg;
    COLORREF m_clrText;
    COLORREF m_clrHoverText;    // hover 시 텍스트 색상 (#333D4B 등)
    BOOL     m_bUseHoverText;  // hover text color 사용 여부
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

    BOOL     m_bLoading;
    CString  m_strBaseText;
    CString  m_strLoadingText;

    // Cached font handle (avoids CreateFontIndirect on every paint call)
    HFONT    m_hCachedFont;
    int      m_nCachedFontHeight;
    BOOL     m_bNcStylePrepared;
    bool     m_bLastHover;     // render cache: last drawn hover state
    bool     m_bLastPressed;   // render cache: last drawn pressed state
    bool     m_bLastDisabled;  // render cache: last drawn disabled state
    COLORREF m_clrLastUnderlay; // render cache: last drawn underlay color
    BOOL     m_bRenderDirty;    // render cache: TRUE forces full re-render on next DrawItem

    // Cached back-buffer (reused across DrawItem calls)
    CDC      m_memDC;
    CBitmap  m_memBmp;
    CSize    m_memBmpSize;
    std::wstring m_wstrTextCache;  // cached wide text (updated when window text changes)
    CString      m_strTextKey;     // text key for cache invalidation

    // Loading base cache: bg + text + static arc (no rotating arc)
    CDC            m_memDCBase;
    CBitmap        m_memBmpBase;
    BOOL           m_bLoadingBaseDirty;
    Gdiplus::RectF m_cachedSpRc;
    Gdiplus::Color m_cachedSpColor;
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

    BOOL     m_bLoading;
    CString  m_strBaseText;
    CString  m_strLoadingText;
    std::wstring m_wstrTextCache;  // cached wide text
    CString      m_strTextKey;     // text key for cache invalidation

    // Cached font handle (avoids CreateFontIndirect on every paint call)
    HFONT    m_hCachedFont;
    int      m_nCachedFontHeight;
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

    BOOL m_bToggled;
    BOOL m_bHover;
    BOOL m_bTracking;

    // ()  
    int  m_nTextPx;
    BOOL m_bNoWrapEllipsis;
    HFONT m_hCachedFont;
    int   m_nCachedFontH;
    std::wstring m_wstrTextCache;  // cached wide text
    CString      m_strTextKey;     // text key for cache invalidation
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
    void SetPending(BOOL bPending);

    void SetTextSizePx(int px) { m_nTextPx = px; Invalidate(FALSE); }
    void SetNoWrapEllipsis(BOOL b) { m_bNoWrapEllipsis = b; Invalidate(FALSE); }
    void SetUnderlayColor(COLORREF clr) { m_bUseUnderlay = TRUE; m_clrUnderlay = clr; Invalidate(FALSE); }

protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg LRESULT OnMouseHover(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    DECLARE_MESSAGE_MAP()

private:

    BOOL m_bToggled;
    BOOL m_bHover;
    BOOL m_bTracking;
    BOOL m_bPressed; // 마우스 눌림 상태 저장용 추가

    int  m_nTextPx;
    BOOL m_bNoWrapEllipsis;

    BOOL m_bUseUnderlay;
    COLORREF m_clrUnderlay;

    float    m_fAnimProgress;
    UINT_PTR m_uAnimTimer;

    BOOL     m_bPending;     // TRUE while async operation in progress
    float    m_fSpinAngle;   // spinner arc rotation angle (0..360)
    UINT_PTR m_uSpinTimer;   // 33ms timer for spinner animation
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
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg LRESULT OnPrintClientMsg(WPARAM wParam, LPARAM lParam);
    afx_msg void OnNcPaint();
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

    // GDI+ 텍스트용 캐시 폰트
    CString      m_strDisplayCache;   // text key for kftc_to_wide cache
    std::wstring m_wstrDisplayCache;  // cached wide string for DrawTextW
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

    BOOL     m_bLoading;
    CString  m_strBaseText;
    CString  m_strLoadingText;

    // Cached back-buffer (reused across PaintComboToDC calls)
    CDC      m_memDC;
    CBitmap  m_memBmp;
    CSize    m_memBmpSize;
};

// ========================================
// CSkinnedEdit -    θ(Edit)
//  - θ//Ŀ β Combo  
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
    CString GetBaseText() const { return m_strBaseText; } // 버튼 스타일 명시 설정
    void SetValidationError(BOOL bError);
    BOOL HasValidationError() const { return m_bValidationError; }
    void SetNumericOnly(BOOL b) { m_bNumericOnly = b; }
    void SetUnitText(LPCTSTR szUnit, int rightPx = 30);

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
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg LRESULT OnImeComposition(WPARAM wParam, LPARAM lParam);
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
    BOOL m_bValidationError;
    BOOL m_bNumericOnly;

    int  m_nRadius;
    int  m_nTextPx;

    // Underlay
    BOOL     m_bUseUnderlayBg;
    COLORREF m_clrUnderlayBg;
    CBrush   m_brUnderlay; // for WM_CTLCOLOR (text background)
    COLORREF m_clrBrushBg;  // cached brush color

    BOOL     m_bNcStylePrepared;
    BOOL     m_bLoading;
    CString  m_strBaseText;
    CString  m_strLoadingText;
    CString  m_strUnitText;
    int      m_nUnitRightPx;

    // Cached back-buffer (reused across OnPaint calls)
    CDC      m_memDC;
    CBitmap  m_memBmp;
    CSize    m_memBmpSize;
};

// ============================================================
// CModernTabCtrl - Ŀ  
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
    static const int kBarH = 44;

protected:
    afx_msg void    OnPaint();
    afx_msg BOOL    OnEraseBkgnd(CDC* pDC);
    afx_msg void    OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void    OnMouseMove(UINT nFlags, CPoint pt);
    afx_msg void    OnMouseLeave();
    afx_msg LRESULT OnMouseHover(WPARAM wp, LPARAM lp);
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    DECLARE_MESSAGE_MAP()

private:
    std::vector<ModernTabItem> m_items;
    int  m_nSel;
    int  m_nHover;
    bool m_bTrack;
    int  m_nCachedTabDpi;

    CFont  m_font;
    CFont  m_fontBold;
    CBrush m_brushBg;

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
    virtual ~CInfoIconButton() { if (m_hCachedFont) { ::DeleteObject(m_hCachedFont); m_hCachedFont = NULL; } }

    void SetUnderlayColor(COLORREF clr) { m_bUseUnderlay = TRUE; m_clrUnderlay = clr; }

protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
     afx_msg BOOL OnEraseBkgnd(CDC* pDC);
     afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
DECLARE_MESSAGE_MAP()

private:
    BOOL     m_bHover;
    BOOL     m_bTracking;
    BOOL     m_bUseUnderlay;
    COLORREF m_clrUnderlay;
    HFONT    m_hCachedFont;
    int      m_nCachedFontH;
};

// ========================================
// CModernPopover - floating info popover (Toss/Kakao style)
// ========================================
class CModernPopover : public CWnd
{
public:
    CModernPopover();
    virtual ~CModernPopover() { if (s_hMouseHook && s_pPopoverInst == this) { ::UnhookWindowsHookEx(s_hMouseHook); s_hMouseHook = NULL; s_pPopoverInst = NULL; } delete m_pFontFamily; m_pFontFamily = nullptr; if (m_hCachedFontBold) { ::DeleteObject(m_hCachedFontBold); m_hCachedFontBold = NULL; } if (m_hCachedFontNormal) { ::DeleteObject(m_hCachedFontNormal); m_hCachedFontNormal = NULL; } }

    void ShowAt(const CRect& anchorScreenRect, LPCTSTR title, LPCTSTR body, CWnd* pParent);
    void Hide();
    BOOL IsVisible() const { return m_bVisible; }

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    DECLARE_MESSAGE_MAP()

private:
    static void RegisterPopoverClass();
        void AddPopoverPath(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& cardRc, REAL radius, float arrowX, float arrowTipY, float arrowHalfW);
void RefreshLayered();
    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    CString m_strTitle;
    CString m_strBody;
    int     m_nArrowX;
    int     m_nCardW;   // DPI-scaled card width (without shadow padding)
    int     m_nCardH;   // DPI-scaled card height (auto-sized from text)
    int     m_nBlurPad; // extra pad for shadow blur (prevents clipping)
    BOOL    m_bVisible;
    Gdiplus::FontFamily* m_pFontFamily;  // cached once per ShowAt, reused for paint/measure
    HFONT   m_hCachedFontBold;    // GDI font cache for RefreshLayered (bold)
    HFONT   m_hCachedFontNormal;  // GDI font cache for RefreshLayered (normal)
    int     m_nCachedGdiFontDpi;  // DPI at which the above fonts were created
    CRect   m_rcVisibleCard; // screen rect of visible card+arrow (excludes shadow padding)
    BYTE     m_nFadeAlpha;   // 0=transparent, 255=fully visible
    UINT_PTR m_uFadeTimer;   // fade animation timer ID
    static HHOOK           s_hMouseHook;
    static CModernPopover* s_pPopoverInst;

    // Base sizes are in 96-DPI pixels. Actual window size is DPI-scaled.
// Popover height is auto-sized from text (minimum kPopMinH).
    static const int kPopW = 292; // toss-like width
    static const int kPopMinH = 96;
    static const int kArrowH = 10;
    static const int kShadowPad = 16;
    static const int kShadowBlurPad = 12; // extra blur room for layered shadow
    static const int kShadowOffY = 8;
};


// ========================================
// CTrayPopupItem - menu item descriptor for CTrayPopup
// ========================================
struct CTrayPopupItem
{
    int     nCmd;
    CString strText;
    BOOL    bSeparator;
    BOOL    bDanger;
    CTrayPopupItem() : nCmd(0), bSeparator(FALSE), bDanger(FALSE) {}
    CTrayPopupItem(int cmd, LPCTSTR text, BOOL sep = FALSE, BOOL danger = FALSE)
        : nCmd(cmd), strText(text), bSeparator(sep), bDanger(danger) {}
};

// ========================================
// CTrayPopup - custom tray icon popup (Kakao/Toss style)
// ========================================
class CTrayPopup : public CWnd
{
public:
    CTrayPopup();
    virtual ~CTrayPopup();

    void Show(HWND hOwner, const std::vector<CTrayPopupItem>& items, CPoint ptScreen);
    void Hide();
    BOOL IsVisible() const { return m_bVisible; }

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    DECLARE_MESSAGE_MAP()

private:
    static void RegisterPopupClass();
    void        RefreshLayered();
    int         HitTestItem(CPoint ptClient) const;

    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK       s_hHook;
    static CTrayPopup* s_pInst;

    HWND   m_hOwner;
    std::vector<CTrayPopupItem> m_items;
    int    m_nHover;
    BOOL   m_bVisible;
    BOOL   m_bTracking;

    BYTE     m_nFadeAlpha;
    UINT_PTR m_uFadeTimer;

    int m_nItemH;
    int m_nSepH;
    int m_nPadX;
    int m_nRadius;
    int m_nShadowPad;
    int m_nCardW;
    int m_nCardH;

    Gdiplus::FontFamily* m_pFontFamily;
    HFONT m_hFont;
    int   m_nCachedFontDpi;

    static const int kBaseCardW    = 160;
    static const int kBaseItemH    = 32;
    static const int kBaseSepH     = 9;
    static const int kBasePadX     = 16;
    static const int kBaseRadius   = 12;
    static const int kBaseShadPad  = 12;
    static const int kBaseShadOffY = 6;
};