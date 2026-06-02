# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## [중요] 파일 수정 시 CP949 인코딩 규칙

**이 프로젝트의 소스 파일(.cpp, .h, .rc)은 CP949(한국어) 인코딩으로 저장되어야 한다.**

- Edit 또는 Write 도구를 사용하면 한글이 깨진다 — **절대 사용 금지**
- 모든 소스 코드 파일 수정은 반드시 **PowerShell**로 수행한다
- CP949 파일 읽기/쓰기: `[System.Text.Encoding]::GetEncoding(949)`
- UTF-8 without BOM 파일(.iss 등): `[System.Text.UTF8Encoding]::new($false)`
- 한글이 포함된 파일이면 무조건 PowerShell 사용 (Edit/Write 도구 사용 불가)

```powershell
# CP949 파일 읽기
$enc = [System.Text.Encoding]::GetEncoding(949)
$text = [System.IO.File]::ReadAllText("파일경로", $enc)

# CP949 파일 쓰기
[System.IO.File]::WriteAllText("파일경로", $text, $enc)
```

## Project Root
All file edits must use the path `C:\Project\MerchantSetup_OnPaintIcons_Clean_CP949` as the project root.

## Build Commands

**CLI:**
```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" MerchantSetup.vcxproj /p:Configuration=Release /p:Platform=Win32 /t:Build /nologo /v:minimal
```

Output: `Release/MerchantSetup.exe`

**Toolchain constraints:**
- Win32 (x86) only — do not change to x64
- MSVC v143 (VS2022), Windows 10 SDK
- MFC Dynamic linking (`UseOfMfc=Dynamic`)
- Character set: **MultiByte (CP949)** — source files are CP949-encoded Korean
- Precompiled headers disabled (`PrecompiledHeader=NotUsing`)

## Architecture Overview

### Application Flow

`CMerchantSetupApp::InitInstance()` → creates modal `CKFTCOneCAPDlg` (home screen) → user clicks a card button → opens sub-dialog modally → returns to home or exits.

Registry key root: `KFTC_VAN` (set via `SetRegistryKey`).

### Dialog Structure

**`CKFTCOneCAPDlg`** (home screen, `IDD_KFTCONECAP_DIALOG`) — main launcher with 4 card-style buttons:
- Reader Setup card → opens `CReaderSetupDlg`
- Shop Setup card → opens `CShopSetupDlg`
- Transaction card → opens `CTransDlg`
- Receipt Setup card → opens `CSlipSetupDlg`
- Card icons drawn via GDI+ helpers: `DrawReaderIcon`, `DrawShopIcon`, `DrawTransIcon`, `DrawReceiptIcon`
- `CHomeCardButton` — custom `CButton` with hover/press animation (timer-driven)

**`CShopSetupDlg`** (main dialog, `IDD_SHOP_SETUP_DLG`) — 4-tab configuration UI:
- Tab 0: VAN server / card reader settings
- Tab 1: Peripheral devices (signature pad, scanner, MSR, terminal speed)
- Tab 2: System / alarm / hotkey settings
- Tab 3: Merchant data download — hosts `CShopDownDlg`

**`CShopDownDlg`** — scrollable 25-row merchant data grid

**`CTransDlg`** — transaction approval/cancel dialog with `CSegmentCtrl`

**`CKeyinDlg`** — numeric keypad input dialog with `CDimDlg` (screen overlay)

**`CLogTransferDlg`** — log file transfer dialog with date picker

---

## CShopSetupDlg 옵션 추가 체크리스트

새 콤보박스 옵션을 추가할 때 아래 항목을 **모두** 처리해야 한다.

### 1. 익명 namespace (파일 상단 `namespace { ... }`)
```cpp
static LPCTSTR MY_FIELD = _T("MY_FIELD_NAME");  // 레지스트리 키
static const ComboItem kMyItems[] = {
    { _T("표시명"), _T("저장값") },
};
```

### 2. resource.h — 컨트롤 ID 정의
```cpp
#define IDC_STATIC_MY_LABEL   XXXX
#define IDC_COMBO_MY_COMBO    XXXX+1
```
팝오버 버튼이 있으면 ShopSetupDlg.h에도:
```cpp
#define IDC_BTN_MY_INFO  60XXX  // 60100~60119 범위
```

### 3. MerchantSetup.rc — 다이얼로그 리소스
```
LTEXT "라벨", IDC_STATIC_MY_LABEL, ...
COMBOBOX IDC_COMBO_MY_COMBO, ..., CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
```
비활성화 기본값이면 `WS_DISABLED` 스타일 추가.

### 4. ShopSetupDlg.h
- `CSkinnedComboBox m_comboMyCombo;` 멤버 추가
- 팝오버 버튼: `CInfoIconButton m_btnMyInfo;`
- 스냅샷 구조체: `int cmbMyCombo;`
- `ON_COMMAND_RANGE` 범위에 새 버튼 ID 포함 확인

### 5. ShopSetupDlg.cpp — DoDataExchange
```cpp
DDX_Control(pDX, IDC_COMBO_MY_COMBO, m_comboMyCombo);
```

### 6. InitializeControls()
```cpp
FillCombo(m_comboMyCombo, kMyItems, _countof(kMyItems));

// 라벨 폰트 배열 (lblIds[])
IDC_STATIC_MY_LABEL,

// 콤보 SetFont 배열 (inputControls[])
&m_comboMyCombo,

// SetTextPx 루프 (CSkinnedComboBox* 배열)
&m_comboMyCombo

// 기타
m_comboMyCombo.SetUnderlayColor(bgColor);
RemoveEdges(IDC_COMBO_MY_COMBO);
CreateInfoBtn(m_btnMyInfo, IDC_BTN_MY_INFO);
```

### 7. LoadOptionsFromRegistry()
```cpp
if (GetRegisterData(SEC_SERIALPORT, MY_FIELD, s))
    SelectComboByValue(m_comboMyCombo, kMyItems, _countof(kMyItems), s, 0);
else
    SelectComboByValue(m_comboMyCombo, kMyItems, _countof(kMyItems), _T("기본값"), 0);
```

### 8. SaveOptionsToRegistry()
```cpp
v = GetSelectedComboValue(m_comboMyCombo, kMyItems, _countof(kMyItems), _T("기본값"));
AfxGetApp()->WriteProfileString(SEC_SERIALPORT, MY_FIELD, v);
```

### 9. TakeSnapshot() / HasUnsavedChanges()
```cpp
// TakeSnapshot:
m_snap.cmbMyCombo = m_comboMyCombo.GetCurSel();
// HasUnsavedChanges:
if (m_comboMyCombo.GetCurSel() != m_snap.cmbMyCombo) return TRUE;
```

### 10. ShowTab() — s_tab1[] 또는 s_tab2[] 배열
```cpp
IDC_STATIC_MY_LABEL, IDC_COMBO_MY_COMBO,
```

### 11. ApplyLayout() — 좌표 배치
```cpp
Move(IDC_STATIC_MY_LABEL, x, y, w, capH);
PlaceInfoBtn(m_btnMyInfo, IDC_STATIC_MY_LABEL, x, y, capH);
Move(IDC_COMBO_MY_COMBO, x, y + capH + capG, w, FIELD_H);
```

### 12. 팝오버 버튼 가시성 (ShowTab 내부)
```cpp
m_btnMyInfo.ShowWindow(nTab == 1 ? SW_SHOW : SW_HIDE);
```

### 13. 팝오버 메시지 (OnInfoButtonClicked의 kTable[])
```cpp
{ &m_btnMyInfo, _T("제목"), _T("설명\n· 항목1\n· 항목2") },
```

### 14. 조건부 활성화 (선택사항)

다른 컨트롤 값에 따라 활성화/비활성화가 필요하면:

- `UpdateToggleDependentEdits()`에 블록 추가
- **반드시 `OnCommand()`의 CBN_SELCHANGE에서 직접 처리**:

```cpp
// OnCommand() 내 CBN_SELCHANGE 케이스
if (code == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBO_TRIGGER)
    UpdateToggleDependentEdits(TRUE);
```

> **⚠️ 주의**: `ON_CBN_SELCHANGE` 메시지맵 핸들러만으로는 작동하지 않는다.
> `CSkinnedComboBox`에 `ON_CONTROL_REFLECT(CBN_SELCHANGE, OnCbnSelchange)`가 등록되어 있어,
> MFC가 알림을 자식에게 먼저 반사(reflect)한다. 자식이 처리(return TRUE)하면
> `CWnd::OnCommand`가 `OnCmdMsg`를 호출하지 않아 부모의 `ON_CBN_SELCHANGE` 핸들러가 **호출되지 않는다**.
> 반드시 `OnCommand()` override 내에서 직접 처리해야 한다.

---

## 현재 구현된 옵션 (Tab 1 — 장치 정보)

| 컨트롤 | 레지스트리 키 | 섹션 | 기본값 | 비고 |
|---|---|---|---|---|
| `m_comboInterlock` | `INTERLOCK` | SERIALPORT | `NORMAL` | 장치 연동 방식 |
| `m_comboTerminalSpeed` | `TERMINAL_SPEED` | SERIALPORT | `115200` | forPOS 선택 시만 활성화 |
| `m_comboSignPadUse` | `SIGNPAD_USE` | SERIALPORT | `YES` | 서명패드 사용 |
| `m_editSignPadPort` | `SIGNPAD` | SERIALPORT | `0` | YES일 때만 활성화 |
| `m_comboSignPadSpeed` | `SIGNPAD_SPEED` | SERIALPORT | `57600` | YES일 때만 활성화 |
| `m_chkScannerUse` | `BARCODE_USE` | SERIALPORT | OFF | 스캐너 사용 |
| `m_editScannerPort` | `BARCODE_PORT` | SERIALPORT | `0` | ON일 때만 활성화 |
| `m_chkMultiVoice` | `MULTIPAD_SOUND` | SERIALPORT | OFF | 멀티패드 음성 출력 |

---

## Key Patterns

**DPI scaling:** 모든 픽셀값은 96-DPI 기준. `ModernUIDpi::Scale(m_hWnd, px)` 또는 로컬 헬퍼 `SX(int px)` 사용.

**Underlay color:** GDI+ 안티앨리어싱 번짐 방지. 모든 커스텀 컨트롤에 `SetUnderlayColor(bgColor)` 호출.

**Hover coalescing:** `SetTimer` (16ms)로 rapid `WM_MOUSEMOVE` 코얼레싱. 제거 금지.

**ON_CONTROL_REFLECT 주의사항:**
`CSkinnedComboBox`에 `ON_CONTROL_REFLECT(CBN_SELCHANGE, OnCbnSelchange)`가 있어
부모의 `ON_CBN_SELCHANGE` 핸들러가 MFC reflection 때문에 호출되지 않는다.
콤보 변경에 반응하는 로직은 반드시 `CShopSetupDlg::OnCommand()` 내에서 처리.

**Child dialog hosting:** `CShopDownDlg`는 `m_staticShopContainer`를 부모로 생성하고 위치를 컨테이너 rect에 맞춘다.

**GDI+ rounded rendering:** `AddRoundRect(GraphicsPath&, RectF, REAL radius)` 공용 헬퍼 사용.

**`CDimDlg`:** `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST` 전화면 반투명 오버레이. `CKeyinDlg` 수명과 동일.

---

## ModernUI Library (`ModernUI.h` / `ModernUI.cpp`)

| Class | Base | Role |
|---|---|---|
| `CModernButton` | `CButton` | 둥근 버튼 hover/pressed 상태 |
| `CModernCheckBox` | `CButton` | 커스텀 체크박스 |
| `CPortToggleButton` | `CButton` | 포트 선택 토글 |
| `CModernToggleSwitch` | `CButton` | ON/OFF 스위치 |
| `CSkinnedComboBox` | `CComboBox` | Owner-drawn 드롭다운 |
| `CSkinnedEdit` | `CEdit` | 둥근 테두리 입력창 |
| `CModernTabCtrl` | `CWnd` | 커스텀 탭바 |
| `CInfoText` | `CStatic` | 읽기 전용 값 표시 |
| `CInfoIconButton` | `CButton` | 팝오버 트리거 "i" 버튼 |
| `CModernPopover` | `CWnd` | 방향 화살표 팝오버 (global mouse hook으로 자동 닫힘) |
| `CSegmentCtrl` | `CWnd` | 세그먼트 탭바 (CTransDlg용) |
| `CHomeCardButton` | `CButton` | 애니메이션 카드 버튼 |

**Helper namespaces:**
- `ModernUITheme` — `KFTCInputTheme` 공유 구조체
- `ModernUIGfx` — GDI+ 시작/종료 (`EnsureGdiplusStartup` / `ShutdownGdiplus`)
- `ModernUIFont` — 폰트 리소스 관리 (`EnsureFontsLoaded` / `ShutdownFonts`)
- `ModernUIDpi` — `Scale(hwnd, px)` / `ScaleF(hwnd, px)`

### Color Palette

- `KFTC_PRIMARY` = `BLUE_500` = `RGB(0, 100, 221)`
- `KFTC_TEXT_DARK` = `BLUE_800` = `RGB(6, 52, 109)`
- `KFTC_BG_LIGHT` = `BLUE_50` = `RGB(235, 244, 255)`
- `KFTC_BORDER` = `RGB(214, 228, 247)`

---

## Resources

- `MerchantSetup.rc` — CP949 인코딩. 텍스트 에디터 직접 편집 금지, Visual Studio 사용.
- `resource.h` — 모든 리소스 ID `#define`. 새 컨트롤 추가 시 여기 먼저.
- `KFTCOneCAP_Setup.iss` — Inno Setup 인스톨러 (UTF-8 without BOM)

---

## Testing — pywinauto-mcp

이 프로젝트에 Windows GUI 자동화 테스트를 위한 pywinauto-mcp MCP 서버가 설정되어 있다 (`.mcp.json`).

**UI 변경사항 테스트 예시:**
```python
# 창 찾기
automation_windows(action="find", title="가맹점 설정")

# 컨트롤 enabled 상태 직접 확인 (스크린샷 불필요)
automation_elements(action="get_state", window="가맹점 설정", control="단말기 연동 속도ComboBox")
# → { "enabled": true/false }

# 콤보 선택
automation_elements(action="select", window="가맹점 설정", control="장치 연동 방식ComboBox", value="단말기(forPOS)")
```

좌표 계산 없이 컨트롤 이름으로 직접 접근하므로 스크린샷 기반 테스트보다 빠르고 정확하다.
