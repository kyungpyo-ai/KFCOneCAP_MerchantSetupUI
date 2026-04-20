// LogTransferDlg.h
#pragma once
#include "resource.h"
#include "ModernUI.h"
#include <afxdtctl.h>

class CLogTransferDlg : public CDialog
{
    DECLARE_DYNAMIC(CLogTransferDlg)
public:
    CLogTransferDlg(CWnd* pParent = NULL);
    virtual ~CLogTransferDlg();
    enum { IDD = IDD_LOG_TRANSFER_DIALOG };

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnPaint();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnBtnSend();
    afx_msg void OnBtnCancel();
    afx_msg void OnBtnClose();
    afx_msg LRESULT OnNcHitTest(CPoint point);
    DECLARE_MESSAGE_MAP()
private:
    void EnsureFonts();
    void LayoutControls();
    void StyleCalendarColors();
    int  SX(int px) const;
    void SetClientSize(int cx, int cy);


    CModernButton m_btnSend;
    CModernButton m_btnCancel;
    CModernButton m_btnClose;


    // 1. CDateTimeCtrl을 버리고, 직접 그리는 버튼과 팝업 달력을 사용합니다.
    CModernButton m_btnDatePicker;
    CMonthCalCtrl m_calCtrl;
    CString m_strDate;

    void ShowCalendar();
    void HideCalendar();

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
    afx_msg void OnBtnDatePicker();
    afx_msg void OnCalSelect(NMHDR* pNMHDR, LRESULT* pResult);

    CFont  m_fontTitle;
    CFont  m_fontDesc;
    CFont  m_fontLabel;
    CBrush m_brBg;
    CBrush m_brDateBg;
};