@echo off
rem Offline unit test: hashes, key names, ini parsing, speech struct size. No SDK or game needed.
setlocal
cd /d "%~dp0"

set "VS="
"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\shroudmenu_vs.txt" 2>nul
set /p VS=<"%TEMP%\shroudmenu_vs.txt"
del "%TEMP%\shroudmenu_vs.txt" >nul 2>&1
if "%VS%"=="" (
    echo [!] Visual Studio with C++ not found.
    exit /b 1
)
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 || exit /b 1

if not exist build\test mkdir build\test
cl /nologo /std:c++17 /EHsc /W4 /permissive- /DWIN32_LEAN_AND_MEAN /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /Isrc /Fobuild\test\ ^
   tests\unit_test.cpp src\config.cpp /Fe:build\test\unit_test.exe >build\test\build.log 2>&1
if errorlevel 1 (
    type build\test\build.log
    exit /b 1
)
build\test\unit_test.exe
