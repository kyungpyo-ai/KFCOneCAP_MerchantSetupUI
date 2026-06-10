// LoadingDlg.h
#pragma once
#include "ModernUI.h"

// 로딩 스레드 완료 시 notify 창으로 전달되는 메시지
#define WM_LOADING_DONE (WM_USER + 201)

// GDI+ 회전 스피너 팝업.
// Start() -> 워커 스레드 3초 대기(또는 Stop() 신호) -> hwndNotify 에 WM_LOADING_DONE 전달.
// PostNcDestroy() 에서 delete this 로 자동 소멸 ? 외부에서 delete 하지 말 것.
class CLoadingDlg : public CWnd
{
public:
    CLoadingDlg();
    ~CLoadingDlg();

    // pParent   : 중앙 배치 기준 창
    // hwndNotify: 완료 시 WM_LOADING_DONE 을 받을 HWND
    BOOL Start(CWnd* pParent, HWND hwndNotify);

    // 외부에서 스레드를 조기 종료 (부모에 WM_LOADING_DONE 전달)
    void Stop();

    // 취소: 부모 통보 없이 창 닫기
    void Cancel();

protected:
    virtual void PostNcDestroy() override;

private:
    HWND   m_hwndNotify;
    HANDLE m_hStopEvent;
    HANDLE m_hThread;
    int    m_nAngle;

    int SX(int px) const;
    static UINT WINAPI WorkerThread(LPVOID pParam);

    afx_msg int  OnCreate(LPCREATESTRUCT lpCS);
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();
    afx_msg LRESULT OnLoadingDone(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};