## How to run?

Path to vscars:
`C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build`
Alternative: `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat`

`cmake -B [build directory] -S [source directory]`

* CMake extension for VSCode: https://code.visualstudio.com/docs/cpp/CMake-linux
1. Select a Kit
2. Select the compiler you want to use and/or do [Scan for kits]
3. Select variant e.g. Debug, Release etc.

Windows:
  You can just run: `\build.bat`
  Alternative with CMake:
  1. `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
  2. `cmake --build .\build --target ALL_BUILD --config Debug`
  3. `.\build\Debug\handmade_hero.exe`

Linux:
  1. `$ cmake . -B build -G "Unix Makefiles"`
  2. `$ make -C build VERBOSE=1`
