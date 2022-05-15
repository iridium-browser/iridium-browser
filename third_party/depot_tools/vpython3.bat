@echo off
:: Copyright 2019 The Chromium Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

setlocal
for /f %%i in (%~dp0python3_bin_reldir.txt) do set PYTHON3_BIN_RELDIR=%%i

call "%~dp0\cipd_bin_setup.bat" > nul 2>&1
"%~dp0\.cipd_bin\vpython3.exe" -vpython-interpreter "%~dp0\%PYTHON3_BIN_RELDIR%\python3.exe" %*
