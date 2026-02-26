Set-Location 'c:\Users\kftc\Downloads\MerchantSetup_OnPaintIcons_Clean_CP949'
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
$result = & $msbuild 'MerchantSetup.sln' '/p:Configuration=Release' '/p:Platform=x86' '/t:Rebuild' '/v:minimal' 2>&1
$result | Out-File -FilePath 'build_output.txt' -Encoding UTF8
Write-Host 'Build done. Exit code:' $LASTEXITCODE
