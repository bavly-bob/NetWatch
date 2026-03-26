How to run it in VS Code / Antigravity IDE

Prerequisites (both options)
- CMake
- vcpkg (added as a submodule in this repo)
- Qt 6.10.2 (matching your toolchain)
- 
Option A: MSVC + vcpkg (x64-windows)

1. Install prerequisites
- Qt 6.10.2 (MSVC build): C:\Qt\6.10.2\msvc2019_64
- Visual Studio / MSVC toolchain
- Ninja (optional, MSBuild works too)

2. Install Boost.Asio
   .\vcpkg\vcpkg install boost-asio:x64-windows

3. Configure CMake
   NOTE: Make sure C:\msys64\mingw64\bin is NOT in PATH when using MSVC.
   Recommended: run this from "x64 Native Tools Command Prompt for VS 2022".

   cmake -S . -B build `
     -DCMAKE_TOOLCHAIN_FILE=".\vcpkg\scripts\buildsystems\vcpkg.cmake" `
     -DVCPKG_TARGET_TRIPLET=x64-windows `
     -DCMAKE_PREFIX_PATH="C:\Qt\6.10.2\msvc2019_64" `
     -DQt6_DIR="C:\Qt\6.10.2\msvc2019_64\lib\cmake\Qt6"

4. Build and run
   cmake --build build --config Debug
   build\Debug\NetWatch.exe

Option B: MinGW + vcpkg (x64-mingw-dynamic)

1. Install prerequisites
- Qt 6.10.2 (MinGW build): C:\Qt\6.10.2\mingw_64
- MinGW toolchain: C:\Qt\Tools\mingw1310_64\bin
- Ninja: C:\Qt\Tools\Ninja\ninja.exe

2. Install Boost.Asio
   .\vcpkg\vcpkg install boost-asio:x64-mingw-dynamic

3. Configure CMake (do not change this command)
   cmake -S . -B build-mingw `
 -G Ninja `
 -DCMAKE_TOOLCHAIN_FILE="C:/Users/baboa/OneDrive - aucegypt.edu/Desktop/coding/projects/NetWatch/vcpkg/scripts/buildsystems/vcpkg.cmake" `
 -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic `
 -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/mingw_64" `
 -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1310_64/bin/gcc.exe" `
 -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe"

4. Build and run
   cmake --build build-mingw
   build-mingw\NetWatch.exe

Troubleshooting
- If you see `mingw32.lib` errors while using MSVC, your Qt is the MinGW build.
  Install the Qt MSVC build and reconfigure with its `Qt6_DIR`.
- If MSVC picks up Qt headers from MSYS2, remove `C:\msys64\mingw64\bin` from PATH.

How this was set up
- CMake uses vcpkg to supply Boost (Boost::asio).
- MSVC needs /Zc:__cplusplus and /permissive-, which are enabled in CMake for MSVC.
- Boost.Asio is tested using a simple steady_timer that logs with qDebug().
