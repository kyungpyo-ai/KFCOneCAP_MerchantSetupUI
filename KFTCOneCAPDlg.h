#pragma once

#include "resource.h"

class CKFTCOneCAPDlg : public CDialog
{
public:
    enum { IDD = IDD_KFTCONECAP_DIALOG };

    CKFTCOneCAPDlg(CWnd* pParent = NULL);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void OnReaderSetup();
    afx_msg void OnShopSetup();
    afx_msg void OnTrans();
    afx_msg void OnMinimize();
    afx_msg void OnExit();
    afx_msg void OnClose();

    DECLARE_MESSAGE_MAP()
};
