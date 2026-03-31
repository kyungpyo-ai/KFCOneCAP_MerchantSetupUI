// ModernMessageBox.h - Modern 알림창 헤더
#pragma once

#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")

#include "resource.h"
#include "ModernUI.h"     // 기타 ModernUI 유틸리티

// 메시지 박스 타입
enum class ModernMsgBoxType
{
    Info,       // 정보 (파란색)
    Warning,    // 경고 (노란색/주황색)
    Error,      // 오류 (빨간색)
    Success,    // 성공 (초록색)
    Question    // 질문 (파란색)
};

// 메시지 박스 버튼
enum class ModernMsgBoxButtons
{
    OK,         // 확인만
    OKCancel,   // 확인, 취소
    YesNo,      // 예, 아니오
    YesNoCancel // 예, 아니오, 취소
};

// 결과
enum class ModernMsgBoxResult
{
    OK = IDOK,
    Cancel = IDCANCEL,
    Yes = IDYES,
    No = IDNO
};

class CModernMessageBox : public CDialog
{
    DECLARE_DYNAMIC(CModernMessageBox)

public:
    CModernMessageBox(
        CWnd* pParent = NULL,
        const CString& strMessage = _T(""),
        const CString& strCaption = _T("KFTCGiroCAP"),
        ModernMsgBoxType type = ModernMsgBoxType::Info,
        ModernMsgBoxButtons buttons = ModernMsgBoxButtons::OK
    );
    virtual ~CModernMessageBox();

    enum { IDD = IDD_MODERN_MESSAGEBOX };

    // 정적 함수 - MessageBox 대체용
    static INT_PTR Show(
        const CString& strMessage,
        const CString& strCaption = _T("KFTCGiroCAP"),
        ModernMsgBoxType type = ModernMsgBoxType::Info,
        ModernMsgBoxButtons buttons = ModernMsgBoxButtons::OK,
        CWnd* pParent = NULL
    );

    static INT_PTR Info(const CString& strMessage, CWnd* pParent = NULL);
    static INT_PTR Warning(const CString& strMessage, CWnd* pParent = NULL);
    static INT_PTR Error(const CString& strMessage, CWnd* pParent = NULL);
    static INT_PTR Success(const CString& strMessage, CWnd* pParent = NULL);
    static INT_PTR Question(const CString& strMessage, CWnd* pParent = NULL);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();

    afx_msg void OnPaint();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnDestroy();
    afx_msg void OnYes();
    afx_msg void OnNo();
    afx_msg LRESULT OnNcHitTest(CPoint point); // 드래그 이동 지원을 위해 추가

    DECLARE_MESSAGE_MAP()

private:
    CString m_strMessage;
    CString m_strCaption;
    ModernMsgBoxType m_type;
    ModernMsgBoxButtons m_buttons;

    ULONG_PTR m_gdiplusToken;
    CBrush m_brushBg;
    CFont m_fontTitle;
    CFont m_fontBody;
    // Cached GDI+ objects for DrawIcon (created once in EnsureFonts)
    Gdiplus::Font*          m_pIconFont;
    Gdiplus::Font*          m_pIconFontFallback;
    Gdiplus::StringFormat*  m_pIconFmt;
    Gdiplus::SolidBrush*    m_pIconBgBrush;
    Gdiplus::SolidBrush*    m_pIconSymBrush;

    CModernButton m_btnOK;
    CModernButton m_btnCancel;
    CModernButton m_btnYes;
    CModernButton m_btnNo;

    // 내부 유틸 함수
    void EnsureFonts();
    void MeasureText(CFont& font, const CString& text, int maxW, int& outH);
    int CalculateHeight();
    void SetClientSize(int cx, int cy);
    void SetupButtons(int btnY);
    void DrawIcon(Gdiplus::Graphics& g, Gdiplus::RectF rect, ModernMsgBoxType type);
    int SX(int px) { return ModernUIDpi::Scale(m_hWnd, px); } // DPI 스케일링
};