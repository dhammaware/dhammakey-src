:: SPDX-FileCopyrightText: 2026 Ujudhammo
:: SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
::
:: DhammaKey - a DhammaWare project
:: Homepage: https://dhammaware.org
::
:: Copyright (C) 2026 Ujudhammo.
::
:: TERMS OF USE AND NONCOMMERCIAL COMMITMENT:
:: This software is offered as dana (a free gift) to serve the community, to
:: support study and research, and to help spread the Dhamma. You may use, read,
:: study, modify and contribute improvements to this source code for
:: noncommercial purposes.
::
:: Commercial use, selling, or charging any fee for this software is not
:: permitted. See LICENSE.
::
:: The full, legally binding terms are in the LICENSE file at the project root.
:: Third-party components keep their own licenses; see THIRD-PARTY-NOTICES.txt.
::
:: THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. IN NO EVENT
:: SHALL THE AUTHOR BE LIABLE FOR ANY DAMAGES ARISING FROM ITS USE.

@echo off
REM -------------------------------------------------------------------------
REM  make-latest.bat - tinh ma SHA-256 cua bo cai va tao tep latest-x.y.z.json
REM  cho website (dua len https://dhammaware.org/update/latest.json).
REM
REM  Cach dung (trong cmd, o thu muc installer\windows sau khi wix build):
REM    ..\..\tools\release\make-latest.bat 0.3.34 DhammaKey-0.3.34-x64.msi
REM  Tuy chon thu 3: dia chi thu muc tai ve. Mac dinh (nguoi dung chot 15/11/2026):
REM    https://dhammaware.org/download/dhammakey/x.y.z/
REM  (moi ung dung mot thu muc, moi phien ban mot thu muc, khong ghi de).
REM
REM  Ket qua, nam canh tep .msi:
REM    DhammaKey-0.3.34-x64.msi.sha256   ma SHA-256 (ghi canh nut tai tren web)
REM    latest-0.3.34.json                 muc "dhammakey" cho tep CHUNG
REM                                       https://dhammaware.org/update/latest.json
REM                                       (website ghep vao, giu muc cua ung dung khac)
REM  Chi dung certutil co san tu Windows 7; khong can cai gi them.
REM  Luu y: moi lan bien dich lai, ma SHA-256 doi -> phai chay lai script nay.
REM -------------------------------------------------------------------------
setlocal EnableDelayedExpansion
if "%~2"=="" (
  echo Cach dung: make-latest.bat x.y.z duong-dan\DhammaKey-x.y.z-x64.msi [url-thu-muc-tai]
  exit /b 1
)
set "VER=%~1"
set "FILE=%~f2"
set "NAME=%~nx2"
set "BASE=%~3"
if "%BASE%"=="" set "BASE=https://dhammaware.org/download/dhammakey/%VER%/"
if not "%BASE:~-1%"=="/" set "BASE=%BASE%/"
if not exist "%FILE%" (
  echo Khong thay tep: %FILE%
  exit /b 1
)

REM Dong thu 2 cua certutil la ma bam (Windows 7 co dau cach giua cac byte)
set "HASH="
for /f "skip=1 delims=" %%L in ('certutil -hashfile "%FILE%" SHA256') do (
  if not defined HASH set "HASH=%%L"
)
set "HASH=!HASH: =!"
REM Doi sang chu thuong (DhammaKey chi nhan 0-9 a-f)
for %%C in (a b c d e f) do set "HASH=!HASH:%%C=%%C!"
if "!HASH:~63,1!"=="" goto :bad
if not "!HASH:~64!"=="" goto :bad

> "%FILE%.sha256" echo !HASH! *%NAME%
for %%D in ("%FILE%") do set "OUT=%%~dpDlatest-%VER%.json"
> "!OUT!" (
  echo {
  echo   "dhammakey": {
  echo     "version": "%VER%",
  echo     "url": "%BASE%%NAME%",
  echo     "sha256": "!HASH!",
  echo     "notes": ""
  echo   },
  echo   "version": "%VER%",
  echo   "url": "%BASE%%NAME%",
  echo   "sha256": "!HASH!",
  echo   "notes": ""
  echo }
)
echo.
echo SHA-256 cua %NAME%:
echo   !HASH!
echo Da ghi: %FILE%.sha256
echo Da ghi: !OUT!
echo (Ghi chu "notes" de trong: du an website dien loi tieng Viet/Anh.)
exit /b 0

:bad
echo Khong doc duoc ma SHA-256 tu certutil. Ket qua nhan duoc: !HASH!
exit /b 1
