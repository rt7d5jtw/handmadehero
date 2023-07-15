## How to run?

`cmake -B [build directory] -S [source directory]`

Windows: 
  1. `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
  2. `cmake --build .\build --target ALL_BUILD --config Debug`
  3. `.\build\Debug\handmade_hero.exe`
  or
  Run: `\build.bat`

Linux: 
  1. `$ cmake . -B build -G "Unix Makefiles"`
  2. `$ make -C build VERBOSE=1`
