@echo off
REM Build muparserx.lib from source using CMake

echo ========================================
echo Building muparserx.lib
echo ========================================

REM Set up Visual Studio environment
echo [1/4] Setting up MSVC environment...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 (
    echo ERROR: Failed to set up MSVC environment
    exit /b 1
)

REM Check if CMake is available
echo [2/4] Checking CMake...
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found. Please install CMake or add it to PATH
    echo You can install it from: https://cmake.org/download/
    exit /b 1
)
cmake --version

REM Navigate to muparserx directory
cd external-lib\muparserx\muparserx-4.0.12

REM Create build directory
echo [3/4] Configuring CMake...
if not exist "build" mkdir build
cd build

REM Configure with CMake (Release build, static library)
REM Add policy flag for CMake 4.x compatibility with old CMakeLists.txt
REM IMPORTANT: Add MUP_USE_WIDE_STRING define to match Editor.pro configuration
cmake .. -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_CXX_FLAGS="/DMUP_USE_WIDE_STRING"
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    cd ..\..\..\..
    exit /b 1
)

REM Build using MSBuild
echo [4/4] Building muparserx (this may take a few minutes)...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed
    cd ..\..\..\..
    exit /b 1
)

REM Check if library was created
if exist "Release\muparserx.lib" (
    echo.
    echo ========================================
    echo SUCCESS: muparserx.lib built!
    echo Location: %CD%\Release\muparserx.lib
    echo ========================================

    REM Copy to external-lib root for easier access
    cd ..
    if not exist "lib" mkdir lib
    copy "build\Release\muparserx.lib" "muparserx.lib" >nul
    echo Copied to: %CD%\muparserx.lib

    cd ..\..\..\
) else (
    echo.
    echo ERROR: muparserx.lib not found after build
    cd ..\..\..\
    exit /b 1
)

echo.
echo muparserx.lib is ready for Editor build
