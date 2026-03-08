$enc = [System.Text.Encoding]::GetEncoding(949)
$path = 'c:\Users\user\KFCOneCAP_MerchantSetupUI\ReaderSetupDlg.cpp'
$content = [System.IO.File]::ReadAllText($path, $enc)

$before = $content

# 1. OnPaint: mainCard margin SX(18) -> SX(20)
#    "CRect mainCard(rc.left + margin, ..." 앞의 margin 선언 변경
#    두 곳에 "const int margin = SX(18);" 존재 (OnPaint, CalcLayoutRects)
$content = $content -replace 'const int margin = SX\(18\);', 'const int margin = SX(20);'

# 2. CalcLayoutRects: sectionLeft/Right inner padding SX(12) -> SX(16)
$content = $content -replace 'const int sectionLeft = inner\.left \+ SX\(12\);', 'const int sectionLeft = inner.left + SX(16);'
$content = $content -replace 'const int sectionRight = inner\.right - SX\(12\);', 'const int sectionRight = inner.right - SX(16);'

if ($content -ne $before) {
    [System.IO.File]::WriteAllText($path, $content, $enc)
    Write-Host "Done"
} else {
    Write-Host "No changes made - check patterns"
}
