@echo off
:: SPDX-FileCopyrightText: 2026 Ujudhammo
:: SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
rem DhammaKey - dung tron bo: x86 + x64 + bo cai .msi + .sha256 + latest.json
rem Chay trong "Developer Command Prompt for VS":  tools\release\build-all.bat
rem So phien ban lay tu CMakeLists.txt; doi chieu voi installer\windows\DhammaKey.wxs.
setlocal
cd /d "%~dp0..\.."
set V=
for /f "tokens=3" %%a in ('findstr /r /c:"^project(DhammaKey VERSION" CMakeLists.txt') do set V=%%a
if "%V%"=="" ( echo [LOI] Khong doc duoc so phien ban trong CMakeLists.txt & exit /b 1 )
findstr /r /c:"DkVersion = .%V%." installer\windows\DhammaKey.wxs >nul || ( echo [LOI] DhammaKey.wxs khong ghi DkVersion = "%V%" - chep lai goi ma moi nhat & exit /b 1 )
echo ===== DhammaKey %V% =====
taskkill /f /im DhammaKeySettings.exe >nul 2>nul
cmake -S . -B build-x86 -A Win32 || goto :fail
cmake --build build-x86 --config Release || goto :fail
cmake -S . -B build-x64 -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64 || goto :fail
cmake --build build-x64 --config Release || goto :fail
build-x64\Release\dk_tests.exe || goto :fail
cd installer\windows
wix build DhammaKey.wxs -arch x64 -d Arch=x64 -d BinX64=..\..\build-x64\platform\windows\Release -d BinX86=..\..\build-x86\platform\windows\Release -d SettingsDir=..\..\build-x64\settings\Release -o DhammaKey-%V%-x64.msi || goto :fail
call ..\..\tools\release\make-latest.bat %V% DhammaKey-%V%-x64.msi || goto :fail
echo.
echo ===== XONG: installer\windows\DhammaKey-%V%-x64.msi =====
echo Dung chay wix build lai sau buoc nay (ma SHA-256 se khong con khop).
exit /b 0
:fail
echo.
echo ===== [LOI] Dung o buoc tren. Chup man hinh gui Claude. =====
exit /b 1
