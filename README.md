# NetWatch

NetWatch is a distributed system monitoring project with two apps:

- `NetWatch` (Qt Dashboard): central UI for live device monitoring, incident tracking, and AI triage.
- `NetWatchAgent` (C++ Agent): runs on monitored machines and streams telemetry to the dashboard.

The system uses Boost.Asio networking, a shared JSON payload format, and SQLite-backed incident history.

## Features

- Live CPU and RAM monitoring per device.
- Process list monitoring (PID, name, CPU%, memory) with search/filter in the dashboard.
- Multi-device view with connection status and live refresh.
- Incident persistence in SQLite with severity classification.
- Recent incident history panel in the dashboard.
- AI incident analysis for selected devices (OpenAI/OpenRouter compatible chat-completions flow).
- Cross-platform agent monitors for Windows and Linux.

## Architecture

- `agent/`: collects system + process telemetry and sends periodic updates.
- `dashboard/`: Qt Widgets desktop app for visualization, incident history, and AI analysis.
- `networking/`: shared TCP client/server/connection/protocol layer.
- `common/`: shared types, config constants, and JSON serializer/deserializer.
- `tests/`: GoogleTest protocol and connection coverage.

Data flow:

1. Agent samples metrics every `POLL_INTERVAL_MS` (currently 2000 ms).
2. Agent serializes `SystemStats` to JSON and frames it as `[4-byte length][payload]`.
3. Dashboard server receives and parses packets, then updates UI and DB.
4. Optional AI analysis is requested for the selected device and stored with the related incident.

## Tech Stack

- C++17
- Qt 6 (Widgets, Network, Sql)
- Boost.Asio (`Boost::system`)
- SQLite via Qt SQL module
- CMake
- vcpkg
- GoogleTest

## Prerequisites

- CMake 3.21+
- Git (with submodules)
- Qt 6.10.2 matching your compiler toolchain
- vcpkg submodule initialized

Initialize submodules:

```powershell
git submodule update --init --recursive
```

## Build and Run (Windows)

### Option A: MSVC + vcpkg (`x64-windows`)

1. Install prerequisites:
- Qt 6.10.2 MSVC build: `C:\Qt\6.10.2\msvc2019_64`
- Visual Studio / MSVC toolchain
- Ninja (optional)

2. Install dependency:

```powershell
.\vcpkg\vcpkg install boost-asio:x64-windows
```

3. Configure:

```powershell
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE=".\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.10.2\msvc2019_64" `
  -DQt6_DIR="C:\Qt\6.10.2\msvc2019_64\lib\cmake\Qt6"
```

4. Build:

```powershell
cmake --build build --config Debug
```

5. Run dashboard:

```powershell
build\Debug\NetWatch.exe
```

6. Run agent (same machine):

```powershell
build\Debug\NetWatchAgent.exe 127.0.0.1
```

### Option B: MinGW + vcpkg (`x64-mingw-dynamic`)

1. Install prerequisites:
- Qt 6.10.2 MinGW build: `C:\Qt\6.10.2\mingw_64`
- MinGW toolchain: `C:\Qt\Tools\mingw1310_64\bin`
- Ninja: `C:\Qt\Tools\Ninja\ninja.exe`

2. Install dependency:

```powershell
.\vcpkg\vcpkg install boost-asio:x64-mingw-dynamic
```

3. Configure:

```powershell
cmake -S . -B build-mingw `
  -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/baboa/OneDrive - aucegypt.edu/Desktop/coding/projects/NetWatch/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/mingw_64" `
  -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1310_64/bin/gcc.exe" `
  -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe"
```

4. Build:

```powershell
cmake --build build-mingw
```

5. Run dashboard:

```powershell
build-mingw\NetWatch.exe
```

6. Run agent:

```powershell
build-mingw\NetWatchAgent.exe 127.0.0.1
```

## AI Analysis Setup

Set API key:

```powershell
$env:OPENAI_API_KEY="your_key_here"
```

Optional model override:

```powershell
$env:NETWATCH_LLM_MODEL="gpt-4o-mini"
```

Notes:

- If the key prefix is `sk-or-`, NetWatch routes to OpenRouter endpoint automatically.
- If no key is set, dashboard still works for monitoring, but AI analysis is disabled.

## Running Tests

If GTest is available to CMake, test binaries are generated:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Or for MinGW build:

```powershell
ctest --test-dir build-mingw --output-on-failure
```

## Troubleshooting

- If you get `mingw32.lib` linker errors with MSVC, you are likely using MinGW Qt with MSVC. Use matching Qt build.
- If MSVC picks headers/libraries from MSYS2 unexpectedly, remove `C:\msys64\mingw64\bin` from `PATH`.
- Ensure dashboard and agent both use the same port (`DEFAULT_PORT` in `common/config.hpp`).

## Credits

- Youanes: Frontend (dashboard UI/UX).
- Malak: JSON serialization and Agents.
- Bavly: Networking, AI integration, and Database.
