@echo off
setlocal

set "ENV_CACHE=%~dp0.msvc_env.bat"
set "BUILD_DIR=%~dp0build"

:: -----------------------------------------------------------------------------
:: Command-Line Argument Handling
:: -----------------------------------------------------------------------------
if /i "%~1"=="clean" (
    echo Cleaning build artifacts and environment cache...
    if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
    if exist "%ENV_CACHE%" del /f /q "%ENV_CACHE%"
    if exist "%~dp0.msvc_cache.bat" del /f /q "%~dp0.msvc_cache.bat"
    if exist "%~dp0msvc_debug_log.txt" del /f /q "%~dp0msvc_debug_log.txt"
    echo Cleanup complete.
    exit /b 0
)

:: -----------------------------------------------
:: Locate Visual Studio and initialize environment
:: -----------------------------------------------
if defined DevEnvDir goto :EnvironmentReady

:: If cached environment exists, load it directly
if exist "%ENV_CACHE%" (
    call "%ENV_CACHE%"
    if defined DevEnvDir goto :EnvironmentReady
)

:: Locate vcvars if cache is empty
echo MSVC environment cache not found. Generating %ENV_CACHE%...

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
            set "VCVARS_SCRIPT=%%i\VC\Auxiliary\Build\vcvars64.bat"
            set "VCVARS_ARG="
            goto :GenerateCache
        )
    )
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvars32.bat" (
            set "VCVARS_SCRIPT=%%i\VC\Auxiliary\Build\vcvars32.bat"
            set "VCVARS_ARG="
            goto :GenerateCache
        )
    )
)

:: Fallback directory search for standard install locations
for %%D in ("%ProgramFiles%" "%ProgramFiles(x86)%" "C:\Program Files" "C:\Program Files (x86)") do (
    for %%Y in (2022 2019 2017) do (
        for %%E in (Community Professional Enterprise BuildTools) do (
            if exist "%%~D\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars64.bat" (
                set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars64.bat"
                set "VCVARS_ARG="
                goto :GenerateCache
            )
        )
    )
)

echo Error: Could not locate a working Visual Studio installation. >&2
exit /b 1

:GenerateCache
cmd /v:on /c "call "%VCVARS_SCRIPT%" %VCVARS_ARG% >nul && (echo @echo off& echo set "DevEnvDir=!DevEnvDir!"& echo set "INCLUDE=!INCLUDE!"& echo set "LIB=!LIB!"& echo set "LIBPATH=!LIBPATH!"& echo set "PATH=!PATH!")" > "%ENV_CACHE%"

call "%ENV_CACHE%"

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
