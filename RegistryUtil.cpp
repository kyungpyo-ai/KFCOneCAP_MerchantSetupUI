// RegistryUtil.cpp - 레지스트리 저장/불러오기 유틸 (CP949)

#include "stdafx.h"
#include "RegistryUtil.h"
#include <windows.h>

bool GetRegisterData(LPCTSTR lpSection, LPCTSTR lpField, CString& outValue)
{
    outValue.Empty();

    if (!lpSection || !*lpSection || !lpField || !*lpField)
        return false;

    // Word 스펙 기준 고정 경로:
    // HKCU\Software\KFTC_VAN\KFTCOneCAP\<SECTION>\FIELD
    CString subKey;
    subKey.Format(_T("Software\\KFTC_VAN\\KFTCOneCAP\\%s"), lpSection);

    HKEY hKey = NULL;
    LONG r = RegOpenKeyEx(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey);
    if (r != ERROR_SUCCESS)
    {
        outValue = _T("");
        return false;
    }

    DWORD type = 0;
    DWORD cbData = 0;

    // 존재 여부 + 길이 확인
    r = RegQueryValueEx(hKey, lpField, NULL, &type, NULL, &cbData);
    if (r != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        outValue = _T("");
        return false;
    }

    if (type == REG_SZ || type == REG_EXPAND_SZ)
    {
        CString tmp;
        // cbData: bytes (includes null terminator)
        int cch = (int)(cbData / sizeof(TCHAR));
        if (cch < 1) cch = 1;
        LPTSTR buf = tmp.GetBuffer(cch + 1);
        memset(buf, 0, (cch + 1) * sizeof(TCHAR));

        r = RegQueryValueEx(hKey, lpField, NULL, &type,
                            (LPBYTE)buf, &cbData);

        tmp.ReleaseBuffer();
        RegCloseKey(hKey);

        if (r != ERROR_SUCCESS)
        {
            outValue = _T("");
            return false;
        }

        outValue = tmp;  // ""도 정상 값
        return true;     // 값 존재
    }
    else if (type == REG_DWORD && cbData == sizeof(DWORD))
    {
        DWORD dw = 0;
        r = RegQueryValueEx(hKey, lpField, NULL, &type,
                            (LPBYTE)&dw, &cbData);
        RegCloseKey(hKey);

        if (r != ERROR_SUCCESS)
        {
            outValue = _T("");
            return false;
        }

        outValue.Format(_T("%u"), (UINT)dw);
        return true;
    }

    RegCloseKey(hKey);
    outValue = _T("");
    return false;
}
