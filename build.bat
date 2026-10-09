@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem ---------------------------------------------------------------------------
rem  Builds CS2_External (Release x64).
rem
rem  Usage:  build.bat [Release|Debug] [toolset]
rem    toolset  optional platform toolset override (v143 = VS 2022, v145 = VS 2026).
rem             Detected from the installed Visual Studio when omitted.
rem  Output: x64\<Configuration>\CS2_External.exe
rem ---------------------------------------------------------------------------

cd /d "%~dp0"

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"
set "TOOLSET=%~2"

rem --- locate Visual Studio with vswhere ---------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Install Visual Studio with the "Desktop development with C++" workload.
    goto :fail
)

set "VSPATH="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do set "VSPATH=%%i"
if "%VSPATH%"=="" (
    echo [ERROR] No Visual Studio installation with MSBuild found.
    goto :fail
)

set "MSBUILD=%VSPATH%\MSBuild\Current\Bin\amd64\MSBuild.exe"
if not exist "%MSBUILD%" set "MSBUILD=%VSPATH%\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (
    echo [ERROR] MSBuild.exe not found under "%VSPATH%".
    goto :fail
)

rem --- pick the platform toolset -------------------------------------------------
if "%TOOLSET%"=="" (
    set "VSVER="
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -property installationVersion`) do set "VSVER=%%i"
    for /f "tokens=1 delims=." %%a in ("!VSVER!") do set "VSMAJOR=%%a"
    if "!VSMAJOR!"=="17" set "TOOLSET=v143"
    if "!VSMAJOR!"=="18" set "TOOLSET=v145"
)

set "TOOLSETARG="
if not "%TOOLSET%"=="" set "TOOLSETARG=-p:PlatformToolset=%TOOLSET%"

echo Visual Studio : %VSPATH%
echo Configuration : %CONFIG% x64
echo Toolset       : %TOOLSET%
echo.

"%MSBUILD%" CS2_External.sln -t:Build -p:Configuration=%CONFIG% -p:Platform=x64 %TOOLSETARG% -v:m -nologo
if errorlevel 1 goto :fail

echo.
echo [OK] Built: %~dp0x64\%CONFIG%\CS2_External.exe
if "%CI%"=="" if "%~3"=="" pause
exit /b 0

:fail
echo.
echo [FAILED] Build did not succeed.
if "%CI%"=="" pause
exit /b 1
