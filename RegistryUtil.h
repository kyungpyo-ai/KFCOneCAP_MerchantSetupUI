// RegistryUtil.h - 레지스트리 저장/불러오기 유틸 (CP949)
#pragma once

#include <afxwin.h>

// GetRegisterData
// - 레지스트리에 값이 존재하면 true, outValue에 저장된 값을 반환(빈 문자열 포함)
// - 값이 존재하지 않으면 false, outValue는 ""로 반환
bool GetRegisterData(LPCTSTR lpSection, LPCTSTR lpField, CString& outValue);
