# CLAUDE.md

이 파일은 Claude Code(claude.ai/code)가 이 저장소에서 작업할 때 참고하는 안내 문서입니다.

## 빌드 방법

**Visual Studio (GUI):** MerchantSetup.sln 열기 -> 빌드 -> Release|Win32

**MSBuild (CLI):**
```
msbuild MerchantSetup.sln /p:Configuration=Release /p:Platform=Win32
msbuild MerchantSetup.sln /p:Configuration=Debug /p:Platform=Win32
```

출력 파일: Release/MerchantSetup.exe 또는 Debug/MerchantSetup.exe

- 대상 플랫폼: Win32 (x86 전용), Windows 10 SDK, MSVC v143 (VS2022)
- MFC: 동적 연결 (UseOfMfc=Dynamic)
- 문자셋: MultiByte (CP949) - 소스 파일은 CP949(한국어) 인코딩. UTF-8로 변환 금지.
- 미리 컴파일된 헤더 미사용 (PrecompiledHeader=NotUsing)

## 아키텍처

**실행 흐름:** CMerchantSetupApp::InitInstance() -> CShopSetupDlg 모달 다이얼로그 생성 -> 다이얼로그 종료 시 앱 종료.

### 다이얼로그 구조

- **CShopSetupDlg** (메인 다이얼로그, IDD_SHOP_SETUP_DLG): 4탭 구성
  - 탭 0: VAN 서버 / 카드결제 설정
  - 탭 1: 주변기기 (사인패드, 스캐너, MSR)
  - 탭 2: 시스템 / 알람 / 단축키 설정
  - 탭 3: 가맹점 다운로드 - CShopDownDlg를 CStatic 컨테이너 안에 자식 다이얼로그로 삽입

- **CShopDownDlg** (IDD_SHOP_DOWN_DIALOG): 가맹점 25행 스크롤 목록.
  각 행에 제품ID / 사업자번호 / 비밀번호 / 가맹점명 입력란과 다운로드 버튼.
  컨트롤은 CreateControlsOnce()에서 동적 생성, LayoutControls()에서 배치.

### ModernUI 컨트롤 라이브러리 (ModernUI.h / ModernUI.cpp)

KFTC 블루 테마를 구현하는 커스텀 오너드로우 컨트롤. 모든 렌더링은 GDI+(Gdiplus) 사용.

| 클래스              | 베이스     | 역할                                             |
|---------------------|------------|--------------------------------------------------|
| CModernButton       | CButton    | 둥근 버튼 (호버/누름 상태 포함)                  |
| CModernCheckBox     | CButton    | 커스텀 체크박스                                  |
| CPortToggleButton   | CButton    | 포트 선택 토글 버튼                              |
| CModernToggleSwitch | CButton    | 토스/카카오 스타일 ON/OFF 스위치                 |
| CSkinnedComboBox    | CComboBox  | 오너드로우 드롭다운 (팝업 리스트박스 서브클래싱) |
| CSkinnedEdit        | CEdit      | 둥근 테두리 에디트 (호버/포커스 상태 포함)       |
| CModernTabCtrl      | CWnd       | 커스텀 탭바 (아이콘 직접 드로잉)                 |
| CInfoText           | CStatic    | 읽기 전용 값 표시 (플레이스홀더 지원)            |

**핵심 설계 패턴:**
- 모든 컨트롤은 SetUnderlayColor(COLORREF)를 제공해 둥근 모서리 뒤 배경을 채워 헤일로 현상 방지.
- 레이아웃 픽셀값은 96-DPI 기준으로 작성하고 런타임에 ModernUIDpi::Scale(hwnd, px) / ScaleF()로 스케일.
- GDI+ 수명은 ModernUIGfx::EnsureGdiplusStartup() / ShutdownGdiplus()로 관리. OnInitDialog/OnDestroy에서 호출.
- 전역 입력 테마(KFTCInputTheme)는 ModernUITheme::GetInputTheme()으로 공유. 컨트롤별 로컬 테마 오버라이드 가능.

### 색상 팔레트

ModernUI.h에 BLUE_50 ~ BLUE_900 매크로로 정의, 시맨틱 별칭:
- KFTC_PRIMARY = BLUE_500 = RGB(0,100,221)
- KFTC_TEXT_DARK = BLUE_800
- KFTC_BG_LIGHT = BLUE_50 (다이얼로그 배경 틴트)
- KFTC_BORDER = RGB(214,228,247) (블루 계열 테두리)

입력 테두리 상태: KFTC_INPUT_BORDER_N (기본) -> KFTC_INPUT_BORDER_H (호버) -> KFTC_INPUT_BORDER_F (포커스, 2px).

### 리소스 파일

MerchantSetup.rc (CP949)에 모든 다이얼로그 템플릿 및 리소스 ID 정의.
resource.h에 ID 상수 선언.
