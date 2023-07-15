set msvcdir="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"

set VSCMD_DEBUG=3

call %msvcdir%\vcvars64.bat > msvc_debug_log.txt

REM if not defined DevEnvDir call %msvcdir%vcvars64.bat >nul

echo "Current directory %cd%"

mkdir out
pushd out
cl -Zi ..\main.c user32.lib
popd
