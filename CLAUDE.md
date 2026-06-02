# CLAUDE.md

## [중요] 파일 수정 시 CP949 인코딩 규칙

**소스 파일(.cpp, .h, .rc)은 CP949 인코딩. Edit/Write 도구 사용 금지 — 한글 깨짐.**
모든 수정은 반드시 PowerShell로:

```powershell
$enc = [System.Text.Encoding]::GetEncoding(949)
$text = [System.IO.File]::ReadAllText("파일경로", $enc)
[System.IO.File]::WriteAllText("파일경로", $text, $enc)
```

UTF-8 without BOM 파일(.iss): `[System.Text.UTF8Encoding]::new($false)`

## Project Root
`C:\Project\MerchantSetup_OnPaintIcons_Clean_CP949`

## Build

```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" MerchantSetup.vcxproj /p:Configuration=Release /p:Platform=Win32 /t:Build /nologo /v:minimal
```

- Win32 (x86), MSVC v143, MFC Dynamic, MultiByte(CP949), PrecompiledHeader=NotUsing

## Architecture

`InitInstance()` → `CKFTCOneCAPDlg` (홈) → 카드 클릭 → 서브 다이얼로그 (modal)

- **CShopSetupDlg** — 4탭 설정 UI. Tab0: 결제, Tab1: 장치, Tab2: 시스템, Tab3: 가맹점다운로드
- **CShopDownDlg** — 25행 가맹점 데이터 그리드
- **CKeyinDlg** — 숫자 키패드 + `CDimDlg` 화면 오버레이
- Registry key root: `KFTC_VAN`

---

## CShopSetupDlg 콤보 옵션 추가 시 체크리스트

기존 구현(`m_comboTerminalSpeed` 등)을 grep으로 찾아 패턴을 따른다.

1. `namespace {}` — 레지스트리 키 상수 + `ComboItem kXxx[]` 배열 정의
2. `resource.h` — `IDC_STATIC_*`, `IDC_COMBO_*`, `IDC_BTN_*_INFO` ID 추가
3. **`MerchantSetup.rc`** — `CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP` 필수 ← rc에 없으면 드롭다운 배경 검은색 버그 발생
4. `ShopSetupDlg.h` — 멤버 변수, 팝오버 버튼, 스냅샷 구조체 필드 추가
5. `DoDataExchange` — `DDX_Control` 바인딩
6. `InitializeControls` — FillCombo, lblIds[], inputControls[], SetTextPx 루프, SetUnderlayColor, RemoveEdges, CreateInfoBtn
7. `LoadOptionsFromRegistry` — `SelectComboByValue` (기본값 포함)
8. `SaveOptionsToRegistry` — `GetSelectedComboValue` + `WriteProfileString`
9. `TakeSnapshot` / `HasUnsavedChanges` — 스냅샷 비교 필드 추가
10. `ShowTab` — s_tab1[] 또는 s_tab2[] 배열에 ID 추가, 팝오버 버튼 가시성
11. `ApplyLayout` — Move + PlaceInfoBtn 좌표 배치
12. `OnInfoButtonClicked` kTable[] — 팝오버 설명 텍스트 추가
13. 조건부 활성화 필요 시 → `UpdateToggleDependentEdits()`에 블록 추가

---

## 주요 함정 (Gotchas)

**ON_CONTROL_REFLECT 문제:**
`CSkinnedComboBox`에 `ON_CONTROL_REFLECT(CBN_SELCHANGE)`가 있어 부모의 `ON_CBN_SELCHANGE` 핸들러가 호출되지 않는다.
콤보 변경 반응 로직은 반드시 `OnCommand()` override 안에서 직접 처리:
```cpp
if (code == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBO_FOO)
    UpdateToggleDependentEdits(TRUE);
```

**rc 스타일 누락:**
`CSkinnedComboBox`를 rc에 추가할 때 `CBS_OWNERDRAWFIXED | CBS_HASSTRINGS`가 없으면
드롭다운 아이템 배경이 검은색으로 깨진다. `PreSubclassWindow()`의 런타임 추가만으로는 부족.

**DPI scaling:** 모든 픽셀값은 96-DPI 기준. `SX(int px)` 헬퍼 사용.

**Underlay color:** GDI+ 번짐 방지. 모든 커스텀 컨트롤에 `SetUnderlayColor(bgColor)` 필수.

---

## 현재 구현된 Tab 1 옵션

| 컨트롤 | 레지스트리 키 | 기본값 | 비고 |
|---|---|---|---|
| `m_comboInterlock` | `INTERLOCK` | `NORMAL` | 장치 연동 방식 |
| `m_comboTerminalSpeed` | `TERMINAL_SPEED` | `115200` | forPOS 선택 시만 활성화 |
| `m_comboSignPadUse` | `SIGNPAD_USE` | `YES` | 서명패드 사용 |
| `m_editSignPadPort` | `SIGNPAD` | `0` | YES일 때만 활성화 |
| `m_comboSignPadSpeed` | `SIGNPAD_SPEED` | `57600` | YES일 때만 활성화 |
| `m_chkScannerUse` | `BARCODE_USE` | OFF | 스캐너 사용 |
| `m_editScannerPort` | `BARCODE_PORT` | `0` | ON일 때만 활성화 |
| `m_chkMultiVoice` | `MULTIPAD_SOUND` | OFF | 멀티패드 음성 출력 |

---

## Testing

**기능 검증 (enabled/값/클릭)** → PowerShell UIAutomation 직접 사용 (MCP 오버헤드 없음):

```powershell
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
$root = [Windows.Automation.AutomationElement]::RootElement
$dlg  = $root.FindFirst("Children",
    (New-Object Windows.Automation.PropertyCondition(
        [Windows.Automation.AutomationElement]::NameProperty, "가맹점 설정")))
# AutomationId = resource.h 컨트롤 ID 숫자 (예: IDC_COMBO_TERMINAL_SPEED=2046)
$combo = $dlg.FindFirst("Descendants",
    (New-Object Windows.Automation.PropertyCondition(
        [Windows.Automation.AutomationElement]::AutomationIdProperty, "2046")))
$combo.Current.IsEnabled  # True/False 즉시 반환
```

**시각적 검증 (렌더링/레이아웃/배경색)** → pywinauto-mcp (`.mcp.json` 설정됨):
`mcp__pywinauto-mcp__automation_visual` 로 스크린샷 캡처 후 확인.
