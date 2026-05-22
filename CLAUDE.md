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

**GUI:** Open `MerchantSetup.sln` → Build → Release|Win32

**CLI:**
```
msbuild MerchantSetup.sln /p:Configuration=Release /p:Platform=Win32
msbuild MerchantSetup.sln /p:Configuration=Debug /p:Platform=Win32
```

Output: `Release/MerchantSetup.exe` or `Debug/MerchantSetup.exe`

Convenience scripts: `build.bat` (cmd) or `build.ps1` (PowerShell) both run a Release rebuild via MSBuild.

**Toolchain constraints:**
- Win32 (x86) only — do not change to x64
- MSVC v143 (VS2022), Windows 10 SDK
- MFC Dynamic linking (`UseOfMfc=Dynamic`)
- Character set: **MultiByte (CP949)** — source files are CP949-encoded Korean. Do NOT convert to UTF-8.
- Precompiled headers disabled (`PrecompiledHeader=NotUsing`) — each `.cpp` includes headers explicitly.

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
- System tray support with `NOTIFYICONDATA` and `CTrayPopup`
- Card icons drawn via GDI+ helpers: `DrawReaderIcon`, `DrawShopIcon`, `DrawTransIcon`, `DrawReceiptIcon`
- `CHomeCardButton` — custom `CButton` with hover/press animation (timer-driven, `m_nHoverProgress` / `m_nPressProgress`)

**`CShopSetupDlg`** (main dialog, `IDD_SHOP_SETUP_DLG`) — 4-tab configuration UI:
- Tab 0: VAN server / card reader settings
- Tab 1: Peripheral devices (signature pad, scanner, MSR)
- Tab 2: System / alarm / hotkey settings
- Tab 3: Merchant data download — hosts `CShopDownDlg` as a child dialog inside `m_staticShopContainer`

**`CShopDownDlg`** (child dialog, `IDD_SHOP_DOWN_DIALOG`) — scrollable 25-row merchant data grid:
- Columns: Product ID / Business Reg No / Password / Merchant Name / Secondary Name + Download & Delete buttons per row
- Controls created dynamically in `CreateControlsOnce()`, positioned in `LayoutControls()`
- Uses `CardLayout` struct as single source of truth for column widths (used by both layout and painting)

**`CTransDlg`** (`IDD_TRANS_DIALOG`) — transaction approval/cancel dialog:
- Modes: credit approval/cancel, cash approval/cancel (`ETransMode` enum)
- `CSegmentCtrl` — custom tab-bar widget (see ModernUI section below)
- Fields toggled per mode via `EFieldIndex` enum

**`CKeyinDlg`** (`IDD_KEYIN_DIALOG`) — numeric keypad input dialog (card number / merchant ID / password):
- `m_keyinKind`: 1=card number, 2=merchant/customer number, 3=password
- Keyboard hook (`SetWindowsHookEx WH_KEYBOARD_LL`) captures physical key input
- `CDimDlg` — lightweight Win32 window (not MFC dialog) that dims the screen behind modal dialogs; created/destroyed alongside `CKeyinDlg`
- Cancel and MSR hotkeys loaded from registry `SERIALPORT/CANCEL_HOTKEY` and `SERIALPORT/MSR_HOTKEY`

**`CLogTransferDlg`** (`IDD_LOG_TRANSFER_DIALOG`) — log file transfer dialog with date picker:
- Custom date picker using `CModernButton` + `CMonthCalCtrl` (popup calendar)

**`CReaderSetupDlg`** — card reader hardware settings

**`CSlipSetupDlg`** — receipt/slip printer settings

**`CStartupSelectDlg`** (`IDD_STARTUP_SELECT_DLG`) — startup mode selector (Shop Setup / Reader Setup / KFTC OneCAP)

### ModernUI Library (`ModernUI.h` / `ModernUI.cpp`)

Custom owner-drawn controls with KFTC blue design system. All rendering uses GDI+ (`Gdiplus`).

| Class | Base | Role |
|---|---|---|
| `CModernButton` | `CButton` | Rounded button with hover/pressed states |
| `CModernCheckBox` | `CButton` | Custom checkbox |
| `CPortToggleButton` | `CButton` | Port-selection toggle |
| `CModernToggleSwitch` | `CButton` | ON/OFF switch (TOSS/Kakao style) |
| `CSkinnedComboBox` | `CComboBox` | Owner-drawn dropdown with rounded border |
| `CSkinnedEdit` | `CEdit` | Rounded-border text input |
| `CModernTabCtrl` | `CWnd` | Custom tab bar with icons |
| `CInfoText` | `CStatic` | Read-only value display with placeholder |
| `CInfoIconButton` | `CButton` | Circular "i" button that shows a popover |
| `CModernPopover` | `CWnd` | Floating tooltip with directional arrow |
| `CSegmentCtrl` | `CWnd` | Segmented control / tab bar (used in `CTransDlg`) |
| `CHomeCardButton` | `CButton` | Animated card button with hover/press progress (used in `CKFTCOneCAPDlg`) |

**Helper namespaces:**
- `ModernUITheme` — global `KFTCInputTheme` struct shared by all input controls
- `ModernUIGfx` — GDI+ startup/shutdown (`EnsureGdiplusStartup` / `ShutdownGdiplus`), call in `OnInitDialog`/`OnDestroy`
- `ModernUIFont` — font resource management (`EnsureFontsLoaded` / `ShutdownFonts`), called once from `InitInstance`
- `ModernUIDpi` — `Scale(hwnd, px)` / `ScaleF(hwnd, px)` convert 96-DPI base values to current monitor DPI

### Color Palette

Defined as `BLUE_50`–`BLUE_900` macros in `ModernUI.h`:
- `KFTC_PRIMARY` = `BLUE_500` = `RGB(0, 100, 221)`
- `KFTC_TEXT_DARK` = `BLUE_800` = `RGB(6, 52, 109)`
- `KFTC_BG_LIGHT` = `BLUE_50` = `RGB(235, 244, 255)` (dialog tint)
- `KFTC_BORDER` = `RGB(214, 228, 247)` (section group borders)

Input border states: `KFTC_INPUT_BORDER_N` (normal, 1px) → `KFTC_INPUT_BORDER_H` (hover, 1px) → `KFTC_INPUT_BORDER_F` (focus, 2px, blue).

## Key Patterns

**DPI scaling:** All hard-coded pixel values are 96-DPI base. Convert at runtime with `ModernUIDpi::Scale(m_hWnd, px)`. Never hard-code scaled values. Dialogs use a local `SX(int px)` helper that wraps `ModernUIDpi::Scale`.

**Underlay color (rounded corner halo fix):** GDI+ anti-aliasing bleeds into the area behind rounded corners. Call `SetUnderlayColor(COLORREF)` on every custom control to match the control's container background. When controls move to a different background, update their underlay color.

**Hover coalescing:** `CShopSetupDlg` and `CShopDownDlg` both use `SetTimer` (16ms) to coalesce rapid `WM_MOUSEMOVE` events before redrawing input borders. Do not remove this pattern — it prevents excessive redraws.

**Child dialog hosting:** `CShopDownDlg` is created in `OnInitDialog` with `m_staticShopContainer` (a placeholder Static) as parent. Its position tracks the container rect.

**Pointer arrays in `CShopDownDlg`:** 25 rows × 5 columns = 125 individual `CString` member variables (`m_prdid1`–`m_prdid25`, etc.). `InitPointerArrays()` sets up `CString* m_pPrdid[25]` pointer arrays so row iteration can use loops instead of switch-case.

**GDI+ rounded rendering:** `AddRoundRect(GraphicsPath&, RectF, REAL radius)` is the shared helper for all rounded-rectangle drawing. Use it for any new rounded elements instead of GDI `RoundRect`.

**`CDimDlg` (screen dim overlay):** A bare Win32 window (not MFC) with `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST` that covers the full screen with a semi-transparent black overlay. Created via `Create()` / destroyed via `Destroy()` — lifetime tied to `CKeyinDlg`.

**`CModernPopover`:** Uses `SetWindowsHookEx(WH_MOUSE_LL, ...)` (global mouse hook) to auto-dismiss when the user clicks outside. The hook is installed on `ShowAt()` and removed on hide. The popover is a layered window (`WS_EX_LAYERED`) with a directional arrow painted relative to the anchor button.

**GDI+ font caching in `CKFTCOneCAPDlg`:** `Gdiplus::Font*` members (`m_pGdiFontTitle`, etc.) are created once and reused across `OnPaint` calls to avoid per-frame allocation overhead.

## Resources

- `MerchantSetup.rc` — all dialog layouts and string table (CP949 encoded; edit in Visual Studio, not a text editor)
- `resource.h` — all resource ID `#define`s; edit here when adding new controls or dialogs
- `KFTCOneCAP_Setup.iss` — Inno Setup installer script (UTF-8 without BOM)
- Layout pixel values in `.rc` dialogs are logical units at 96 DPI; actual pixel positioning is recalculated by `ApplyLayout()` / `LayoutControls()` at runtime.