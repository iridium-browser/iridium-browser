@echo off
:: Copyright (c) 2016 The Chromium Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

setlocal

set CIPD_BACKEND=https://chrome-infra-packages.appspot.com
set VERSION_FILE=%~dp0cipd_client_version
set CIPD_BINARY=%~dp0.cipd_client.exe

if not exist "%CIPD_BINARY%" (
  call :CLEAN_BOOTSTRAP
  goto :EXEC_CIPD
)

call :SELF_UPDATE >nul 2>&1
if %ERRORLEVEL% neq 0 (
  echo CIPD client self-update failed, trying to bootstrap it from scratch... 1>&2
  call :CLEAN_BOOTSTRAP
)

:EXEC_CIPD

if %ERRORLEVEL% neq 0 (
  echo Failed to bootstrap or update CIPD client 1>&2
)
if %ERRORLEVEL% equ 0 (
  "%CIPD_BINARY%" %*
)

endlocal & (
  set EXPORT_ERRORLEVEL=%ERRORLEVEL%
)
exit /b %EXPORT_ERRORLEVEL%


:: Functions below.
::
:: See http://steve-jansen.github.io/guides/windows-batch-scripting/part-7-functions.html
:: if you are unfamiliar with this madness.


:CLEAN_BOOTSTRAP
:: To allow this powershell script to run if it was a byproduct of downloading
:: and unzipping the depot_tools.zip distribution, we clear the Zone.Identifier
:: alternate data stream. This is equivalent to clicking the "Unblock" button
:: in the file's properties dialog.
echo.>"%~dp0.cipd_impl.ps1:Zone.Identifier"
powershell -NoProfile -ExecutionPolicy RemoteSigned ^
    -File "%~dp0.cipd_impl.ps1" ^
    -CipdBinary "%CIPD_BINARY%" ^
    -BackendURL "%CIPD_BACKEND%" ^
    -VersionFile "%VERSION_FILE%" ^
  <nul
if %ERRORLEVEL% equ 0 (
  :: Need to call SELF_UPDATE to setup .cipd_version file.
  call :SELF_UPDATE
)
exit /B %ERRORLEVEL%


:SELF_UPDATE
"%CIPD_BINARY%" selfupdate ^
    -version-file "%VERSION_FILE%" ^
    -service-url "%CIPD_BACKEND%"
exit /B %ERRORLEVEL%
