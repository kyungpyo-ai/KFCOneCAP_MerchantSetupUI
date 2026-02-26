@echo off
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" MerchantSetup.sln /p:Configuration=Release /p:Platform=Win32 /t:Rebuild /v:minimal
