@echo off
setlocal

:: -----------------------------------------------------------------------------
:: Compiler and environment paths
:: -----------------------------------------------------------------------------
set MSVC_DIR="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"
set VSCMD_DEBUG=3

:: -----------------------------------------------------------------------------
:: Default build configuration
:: -----------------------------------------------------------------------------
set EXECUTABLE=handmadehero.exe

:: -----------------------------------------------------------------------------
:: Compiler Flags
:: -----------------------------------------------------------------------------
:: -Zi: Generate complete debug information
:: -FC: Full paths in error diagnostics
set DEBUG_FLAGS=-Zi -FC

:: -W4: High warning level
:: -wd4201: Nameless struct/union (used constantly in Windows SDK headers like windows.h)
:: -wd4100: Unreferenced formal parameter (common in Win32 callbacks like WindowProc)
:: -wd4189: Local variable is initialized but not referenced
:: -wd4505: Unreferenced local function has been removed
set WARNING_FLAGS=-W4 -wd4201 -wd4100 -wd4189 -wd4505

:: Optional modern alternative for external headers (MSVC 2019 16.10+):
:: Treats #include <...> as external and silences warnings originating inside them:
:: set EXTERNAL_FLAGS=-external:anglebrackets -external:W0
set COMPILER_FLAGS=%DEBUG_FLAGS% %WARNING_FLAGS%
set LIBS=user32.lib gdi32.lib

:: Load MSVC environment if not already loaded
if not defined DevEnvDir call %MSVC_DIR%\vcvars64.bat > msvc_debug_log.txt

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
