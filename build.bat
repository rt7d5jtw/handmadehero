:: cl compiler flags and options
:: https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-170
:: https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options?view=msvc-170

:: user32.lib https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-creation
:: gdi32.lib https://learn.microsoft.com/en-us/windows/win32/gdi/windows-gdi

set msvcdir="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"

:: default build configuration
set CONFIG=Debug
set EXENAME=handmadehero.exe

set VSCMD_DEBUG=3

call %msvcdir%\vcvars64.bat > msvc_debug_log.txt

REM if not defined DevEnvDir call %msvcdir%vcvars64.bat >nul

echo "Current directory %cd%"

if not exist build\%CONFIG% mkdir build\%CONFIG%
pushd build\%CONFIG%
cl -FC -Zi -Fe:%EXENAME% ..\..\src\main.c user32.lib gdi32.lib
popd
