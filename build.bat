@echo off
setlocal

:: -----------------------------------------------------------------------------
:: Locate Visual Studio and initialize environment (with 32-bit fallback)
:: -----------------------------------------------------------------------------
if defined DevEnvDir goto :EnvironmentReady

:: Try vswhere.exe if it exists
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    :: Look for 64-bit build tools
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
            set "VCVARS_SCRIPT=%%i\VC\Auxiliary\Build\vcvars64.bat"
            set "VCVARS_ARG="
            set "ARCH_MSG=64-bit"
            goto :InitializeEnvironment
        )
    )
    :: Fallback: Look for any VS installation to grab 32-bit build tools
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvars32.bat" (
            set "VCVARS_SCRIPT=%%i\VC\Auxiliary\Build\vcvars32.bat"
            set "VCVARS_ARG="
            set "ARCH_MSG=32-bit (Fallback)"
            goto :InitializeEnvironment
        )
    )
)

:: Scan default directories for modern VS (2022, 2019, 2017)
for %%D in ("%ProgramFiles%" "%ProgramFiles(x86)%" "C:\Program Files" "C:\Program Files (x86)") do (
    for %%Y in (2022 2019 2017) do (
        for %%E in (Community Professional Enterprise BuildTools) do (
            if exist "%%~D\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars64.bat" (
                set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars64.bat"
                set "VCVARS_ARG="
                set "ARCH_MSG=64-bit"
                goto :InitializeEnvironment
            )
            if exist "%%~D\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars32.bat" (
                set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars32.bat"
                set "VCVARS_ARG="
                set "ARCH_MSG=32-bit (Fallback)"
                goto :InitializeEnvironment
            )
        )
    )
)
:: Scan default directories for legacy VS (2015 down to 2008)
for %%D in ("%ProgramFiles(x86)%" "%ProgramFiles%" "C:\Program Files (x86)" "C:\Program Files") do (
    for %%V in (14.0 12.0 11.0 10.0 9.0) do (
        if exist "%%~D\Microsoft Visual Studio %%V\VC\vcvarsall.bat" (
            :: Check if the 64-bit native or cross-compiler actually exists
            if exist "%%~D\Microsoft Visual Studio %%V\VC\bin\amd64\cl.exe" (
                set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio %%V\VC\vcvarsall.bat"
                set "VCVARS_ARG=amd64"
                set "ARCH_MSG=64-bit"
                goto :InitializeEnvironment
            ) else if exist "%%~D\Microsoft Visual Studio %%V\VC\bin\x86_amd64\cl.exe" (
                set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio %%V\VC\vcvarsall.bat"
                set "VCVARS_ARG=x86_amd64"
                set "ARCH_MSG=64-bit"
                goto :InitializeEnvironment
            ) else (
                :: Fallback to 32-bit x86
                set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio %%V\VC\vcvarsall.bat"
                set "VCVARS_ARG=x86"
                set "ARCH_MSG=32-bit (Fallback)"
                goto :InitializeEnvironment
            )
        )
    )
)

echo Error: Could not locate Visual Studio via vswhere or standard directories. >&2
exit /b 1

:InitializeEnvironment
set VSCMD_DEBUG=3
echo Initializing %ARCH_MSG% MSVC environment...
if defined VCVARS_ARG (
    call "%VCVARS_SCRIPT%" %VCVARS_ARG% > msvc_debug_log.txt
) else (
    call "%VCVARS_SCRIPT%" > msvc_debug_log.txt
)

:EnvironmentReady

:: -----------------------------------------------------------------------------
:: Default build configuration
:: -----------------------------------------------------------------------------
set EXECUTABLE=handmadehero.exe

:: -----------------------------------------------------------------------------
:: Compiler Flags
:: -----------------------------------------------------------------------------
set DEBUG_FLAGS=-Zi -FC
set WARNING_FLAGS=-W4 -wd4201 -wd4100 -wd4189 -wd4505
set COMPILER_FLAGS=%DEBUG_FLAGS% %WARNING_FLAGS%
set LIBS=user32.lib gdi32.lib

if not exist build mkdir build
pushd build

:: -----------------------------------------------------------------------------
:: Build Execution
:: -----------------------------------------------------------------------------
echo Compiling %EXECUTABLE%...
cl %COMPILER_FLAGS% -Fe:%EXECUTABLE% ..\src\main.c %LIBS%
set BUILD_STATUS=%ERRORLEVEL%

echo --------------------------------------------------
if %BUILD_STATUS% EQU 0 (
    echo Build DONE
) else (
    echo Build FAILED with error code %BUILD_STATUS%
)
echo --------------------------------------------------

popd
exit /b %BUILD_STATUS%
