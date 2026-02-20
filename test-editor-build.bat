@echo off
REM Test script to build Editor locally (replicating GitHub Actions logic)

echo ========================================
echo Testing Editor Build Locally
echo ========================================

REM Set up Visual Studio environment
echo [1/6] Setting up MSVC environment...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 (
    echo ERROR: Failed to set up MSVC environment
    exit /b 1
)

REM Set Qt path
echo [2/6] Setting Qt path...
set "PATH=C:\Qt\6.7.3\msvc2022_64\bin;%PATH%"
set "Qt6_DIR=C:\Qt\6.7.3\msvc2022_64"

REM Set dependency paths from external-lib
echo [3/6] Setting dependency paths...
set "LIBSNDFILE_INCLUDE=%CD%\external-lib\libsndfile\libsndfile-1.2.2-win64\include"
set "LIBSNDFILE_LIB=%CD%\external-lib\libsndfile\libsndfile-1.2.2-win64\lib"
set "FFTW_INCLUDE=%CD%\external-lib\fftw"
set "FFTW_LIB=%CD%\external-lib\fftw"
set "MUPARSERX_INCLUDE=%CD%\external-lib\muparserx\muparserx-4.0.12\parser"
set "MUPARSERX_LIB=%CD%\external-lib\muparserx\muparserx-4.0.12\build\Release"

REM Verify dependency files exist
echo [4/6] Verifying dependencies...
if not exist "%FFTW_LIB%\libfftw3-3.lib" (
    echo ERROR: libfftw3-3.lib not found in %FFTW_LIB%
    exit /b 1
)
echo   - Found libfftw3-3.lib

if not exist "%LIBSNDFILE_LIB%\sndfile.lib" (
    echo ERROR: sndfile.lib not found in %LIBSNDFILE_LIB%
    exit /b 1
)
echo   - Found sndfile.lib

if not exist "%MUPARSERX_LIB%\muparserx.lib" (
    echo ERROR: muparserx.lib not found in %MUPARSERX_LIB%
    echo Please run build-muparserx.bat first
    exit /b 1
)
echo   - Found muparserx.lib

REM Build Common.lib first (Editor depends on it)
echo [5/6] Checking Common.lib...
if not exist "x64\Release\Common.lib" (
    echo WARNING: Common.lib not found at x64\Release\Common.lib
    echo You may need to build Common project first
    echo Continuing anyway...
)

REM Clean previous Editor build
echo [6/6] Building Editor...
if exist "Editor\Makefile" (
    echo   - Cleaning previous build...
    cd Editor
    nmake clean >nul 2>&1
    del Makefile >nul 2>&1
    cd ..
)

REM Run qmake
echo   - Running qmake...
cd Editor
qmake Editor.pro -spec win32-msvc "CONFIG+=release"
if errorlevel 1 (
    echo ERROR: qmake failed
    cd ..
    exit /b 1
)

REM Run nmake
echo   - Running nmake (this may take a few minutes)...
nmake
if errorlevel 1 (
    echo.
    echo ========================================
    echo ERROR: nmake failed
    echo ========================================
    cd ..
    exit /b 1
)

cd ..

REM Check if Editor.exe was built
if exist "Editor\release\Editor.exe" (
    echo.
    echo ========================================
    echo SUCCESS: Editor.exe built successfully!
    echo Location: %CD%\Editor\release\Editor.exe
    echo ========================================
    dir "Editor\release\Editor.exe"
) else (
    echo.
    echo ========================================
    echo ERROR: Editor.exe not found after build
    echo ========================================
    exit /b 1
)

echo.
echo Test complete. This confirms the build logic works locally.
