# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

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
- Win32 (x86) only ? do not change to x64
- MSVC v143 (VS2022), Windows 10 SDK
- MFC Dynamic linking (`UseOfMfc=Dynamic`)
- Character set: **MultiByte (CP949)** ? source files are CP949-encoded Korean. Do NOT convert to UTF-8.
- Precompiled headers disabled (`PrecompiledHeader=NotUsing`) ? each `.cpp` includes headers explicitly.

> **참고 (한국어):**
> - 대상 플랫폼: Win32 (x86 전용), Windows 10 SDK, MSVC v143 (VS2022)
> - MFC: 동적 연결 (UseOfMfc=Dynamic)
> - 문자셋: MultiByte (CP949) ? 소스 파일은 CP949(한국어) 인코딩. UTF-8로 변환 금지.
> - 미리 컴파일된 헤더 미사용

## Architecture Overview

### Application Flow

`CMerchantSetupApp::InitInstance()` → creates modal `CShopSetupDlg` → blocks until user closes.

### Dialog Structure

**`CShopSetupDlg`** (main dialog, `IDD_SHOP_SETUP_DLG`) ? 4-tab configuration UI:
- Tab 0: VAN server / card reader settings
- Tab 1: Peripheral devices (signature pad, scanner, MSR)
- Tab 2: System / alarm / hotkey settings
- Tab 3: Merchant data download ? hosts `CShopDownDlg` as a child dialog inside `m_staticShopContainer`

**`CShopDownDlg`** (child dialog, `IDD_SHOP_DOWN_DIALOG`) ? scrollable 25-row merchant data grid:
- Columns: Product ID / Business Reg No / Password / Merchant Name / Secondary Name + Download & Delete buttons per row
- Controls created dynamically in `CreateControlsOnce()`, positioned in `LayoutControls()`
- Uses `CardLayout` struct as single source of truth for column widths (used by both layout and painting)

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

**Helper namespaces:**
- `ModernUITheme` ? global `KFTCInputTheme` struct shared by all input controls
- `ModernUIGfx` ? GDI+ startup/shutdown (`EnsureGdiplusStartup` / `ShutdownGdiplus`), call in `OnInitDialog`/`OnDestroy`
- `ModernUIDpi` ? `Scale(hwnd, px)` / `ScaleF(hwnd, px)` convert 96-DPI base values to current monitor DPI

### Color Palette

Defined as `BLUE_50`?`BLUE_900` macros in `ModernUI.h`:
- `KFTC_PRIMARY` = `BLUE_500` = `RGB(0, 100, 221)`
- `KFTC_TEXT_DARK` = `BLUE_800` = `RGB(6, 52, 109)`
- `KFTC_BG_LIGHT` = `BLUE_50` = `RGB(235, 244, 255)` (dialog tint)
- `KFTC_BORDER` = `RGB(214, 228, 247)` (section group borders)

Input border states: `KFTC_INPUT_BORDER_N` (normal, 1px) → `KFTC_INPUT_BORDER_H` (hover, 1px) → `KFTC_INPUT_BORDER_F` (focus, 2px, blue).

## Key Patterns

**DPI scaling:** All hard-coded pixel values are 96-DPI base. Convert at runtime with `ModernUIDpi::Scale(m_hWnd, px)`. Never hard-code scaled values.

**Underlay color (rounded corner halo fix):** GDI+ anti-aliasing bleeds into the area behind rounded corners. Call `SetUnderlayColor(COLORREF)` on every custom control to match the control's container background. When controls move to a different background, update their underlay color.

**Hover coalescing:** `CShopSetupDlg` and `CShopDownDlg` both use `SetTimer` (16ms) to coalesce rapid `WM_MOUSEMOVE` events before redrawing input borders. Do not remove this pattern ? it prevents excessive redraws.

**Child dialog hosting:** `CShopDownDlg` is created in `OnInitDialog` with `m_staticShopContainer` (a placeholder Static) as parent. Its position tracks the container rect. This keeps the 25-row grid's scroll/paint logic isolated.

**Pointer arrays in `CShopDownDlg`:** 25 rows × 5 columns = 125 individual `CString` member variables (`m_prdid1`…`m_prdid25`, etc.). `InitPointerArrays()` sets up `CString* m_pPrdid[25]` pointer arrays so row iteration can use loops instead of switch-case.

**GDI+ rounded rendering:** `AddRoundRect(GraphicsPath&, RectF, REAL radius)` is the shared helper for all rounded-rectangle drawing. Use it for any new rounded elements instead of GDI `RoundRect`.

**`CModernPopover`:** Uses `SetWindowsHookEx(WH_MOUSE_LL, ...)` (global mouse hook) to auto-dismiss when the user clicks outside. The hook is installed on `ShowAt()` and removed on hide. The popover is a layered window (`WS_EX_LAYERED`) with a directional arrow painted relative to the anchor button.

## Resources

- `MerchantSetup.rc` ? all dialog layouts and string table (CP949 encoded; edit in Visual Studio, not a text editor)
- `resource.h` ? all resource ID `#define`s; edit here when adding new controls or dialogs
- Layout pixel values in `.rc` dialogs are logical units at 96 DPI; actual pixel positioning is recalculated by `ApplyLayout()` / `LayoutControls()` at runtime.
