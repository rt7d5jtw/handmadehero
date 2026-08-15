## How to run?

### Windows:

You can just run:
```
.\build.bat && .\out\main.exe
`````

Generate Visual Studio project files for x64:
```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

Build:
```
cmake --build build --config Debug && .\build\Debug\handmadehero.exe
```

### Linux or BSD

Generate Makefiles:
```
cmake . -B build -G "Unix Makefiles"
```

Build:
```
make -C build VERBOSE=1 && ./build/handmadehero`
```

### compile_commands.json

If you dont have compile_commands.json symlinked:
```
ln -s build/compile_commands.json .
```

For vim makeprg:
```
set makeprg=cmake\ --build\ build\ --config\ Debug
```

### Extensions and path for vscars

Path to vscars:
```
C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build
```
Alternative:
```
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat
```

[CMake extension for VSCode](https://code.visualstudio.com/docs/cpp/CMake-linux)
1. Select a Kit
2. Select the compiler you want to use and/or do [Scan for kits]
3. Select variant e.g. Debug, Release etc.

`cmake -B [build directory] -S [source directory]`
