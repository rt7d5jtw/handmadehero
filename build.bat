:: cl compiler flags and options
:: https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-170
:: https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options?view=msvc-170

:: user32.lib https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-creation
:: gdi32.lib https://learn.microsoft.com/en-us/windows/win32/gdi/windows-gdi

set msvcdir="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"

set VSCMD_DEBUG=3

call %msvcdir%\vcvars64.bat > msvc_debug_log.txt

REM if not defined DevEnvDir call %msvcdir%vcvars64.bat >nul

echo "Current directory %cd%"

mkdir out
pushd out
cl -Zi ..\src\main.c user32.lib gdi32.lib
popd
