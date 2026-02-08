@echo off
setlocal enabledelayedexpansion

echo [INFO] Starting Wrapper Build Script for Equalizer APO Editor...

:: 1. Define expected Qt Root (Adjust if user has it elsewhere, but standardizing on C:\Qt is safe for this user based on logs)
set "QT_ROOT=C:\Qt"
set "VCPROJ=Editor.vcxproj"

:: 2. Check for Editor.vcxproj
if not exist "%VCPROJ%" (
    echo [ERROR] %VCPROJ% not found in current directory.
    echo [ERROR] Please run this script from the 'Editor' folder.
    exit /b 1
)

:: 3. Backup the vcxproj before modifying (optional but good practice, though we want these changes to persist for the user)
if not exist "%VCPROJ%.bak" (
    copy "%VCPROJ%" "%VCPROJ%.bak" >nul
    echo [INFO] Backed up %VCPROJ% to %VCPROJ%.bak
)

:: 4. Patch Editor.vcxproj using PowerShell
::    - Fix Qt Include references: ..\..\..\Qt -> C:\Qt
::    - Fix helper references: ..\..\equalizerAPO64 -> ..\
::      (Explanation: ..\..\equalizerAPO64 assumes the parent dir is explicitly named 'equalizerAPO64', which fails in 'eq\equalizerAPO64'. 
::       Actually, '..\..\equalizerAPO64' from 'Editor' means 'Sibling of Parent'.
::       In C:\Learn\equalizerAPO64\Editor: .. is Root. ..\.. is C:\Learn. ..\..\equalizerAPO64 is Root.
::       So '..\..\equalizerAPO64' is just '..\'.
::       In C:\Learn\eq\equalizerAPO64\Editor: .. is Root. ..\.. is C:\Learn\eq. ..\..\equalizerAPO64 (if it exists there) would be ..\.
::       Wait, ..\..\equalizerAPO64 is likely intended to point to the repository root relative to something else or is a legacy path.
::       If I replace '..\..\equalizerAPO64' with '..', it points to the repo root 'Editor/../'.
::       Let's verify line 61: "AdditionalIncludeDirectories>.;..\..\equalizerAPO64;..."
::       If "Editor" is at "Root/Editor", then ".." is "Root".
::       So "..\..\equalizerAPO64" is redundant if it means "Root".
::       However, "..\..\equalizerAPO64" is:
::       Current: Root/Editor.
::       Upper: Root.
::       UpperUpper: ParentOfRoot.
::       UpperUpper/equalizerAPO64: Sibling of Root named equalizerAPO64?
::       Or does it imply the project was typically structured differently?
::       In C:\Learn\equalizerAPO64\Editor:
::       .. = C:\Learn\equalizerAPO64
::       ..\.. = C:\Learn
::       ..\..\equalizerAPO64 = C:\Learn\equalizerAPO64.
::       So yes, valid.
::       In C:\Learn\eq\equalizerAPO64\Editor:
::       .. = C:\Learn\eq\equalizerAPO64
::       ..\.. = C:\Learn\eq
::       ..\..\equalizerAPO64 = C:\Learn\eq\equalizerAPO64.
::       So this path actually works IF the repo folder is named 'equalizerAPO64'.
::       BUT the user cloned it to C:\Learn\eq\equalizerAPO64. So the folder name IS usually equalizerAPO64.
::       Wait, the user said "C:\Learn\eq\equalizerAPO64".
::       This folder IS named equalizerAPO64. So "..\..\equalizerAPO64" SHOULD resolve to the same place.
::       Why change it? To be safe for renaming (e.g. "equalizerAPO64-fork").
::       Replacing '..\..\equalizerAPO64' with '..' makes it robust against folder renaming.
::   
::    - Fix Qt Library references (Absolute paths in vcxproj):
::      Replace 'C:\Learn\equalizerAPO64\external-lib' with '..\external-lib'
::      Replace 'C:\Learn\equalizerAPO64\...' generally with '..\'?

echo [INFO] Patching %VCPROJ% paths...

powershell -Command ^
    "$c = Get-Content '%VCPROJ%';" ^
    "$c = $c -replace '\.\.\\\.\.\\\.\.\\Qt', '%QT_ROOT%';" ^
    "$c = $c -replace 'C:\\Qt', '%QT_ROOT%';" ^
    "$c = $c -replace 'C:/Qt', '%QT_ROOT%';" ^
    "$c | Set-Content '%VCPROJ%'"

:: 5. Setup Build Environment
echo [INFO] Setting up Visual Studio environment...
set "VS_CMD_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"

if exist "%VS_CMD_PATH%" (
    call "%VS_CMD_PATH%"
) else (
    echo [WARNING] Specific VsDevCmd.bat not found. Searching generic...
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        set "VS_PATH=%%i"
    )
    if exist "!VS_PATH!\Common7\Tools\VsDevCmd.bat" (
         call "!VS_PATH!\Common7\Tools\VsDevCmd.bat"
    ) else (
         echo [ERROR] Could not find Visual Studio Dev Command Prompt.
         exit /b 1
    )
)

:: 6. Build Dependencies (Common)
echo [INFO] Building "Common.vcxproj" (Release^|x64)...
msbuild "..\Common.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Rebuild ^
    /p:LIBSNDFILE_INCLUDE="external-lib\libsndfile\libsndfile-1.2.2-win64\include" ^
    /p:FFTW_INCLUDE="external-lib\fftw" ^
    /p:MUPARSERX_INCLUDE="external-lib\muparserx\muparserx-4.0.12\parser" ^
    /p:LIBSNDFILE_LIB="external-lib\libsndfile\libsndfile-1.2.2-win64\lib" ^
    /p:FFTW_LIB="external-lib\fftw" ^
    /p:MUPARSERX_LIB="external-lib\muparserx"

if errorlevel 1 (
    echo [ERROR] Common Build Failed.
    exit /b 1
)

:: 7. Build Editor
if not exist "release" mkdir "release"
if not exist "release\moc_predefs.h.cbt" type nul > "release\moc_predefs.h.cbt"

echo [INFO] Building "Release|x64"...
msbuild %VCPROJ% /p:Configuration=Release /p:Platform=x64 /t:Rebuild

if errorlevel 1 (
    echo [ERROR] Build Failed.
) else (
    echo [SUCCESS] Build Succeeded.
    
    :: 8. Deploy
    echo [INFO] Deploying run-time dependencies...
    
    :: Qt Deployment
    set "QT_BIN=C:\Qt\6.7.3\msvc2022_64\bin"
    if exist "!QT_BIN!\windeployqt.exe" (
        echo [INFO] Running windeployqt...
        "!QT_BIN!\windeployqt.exe" --release --no-translations --no-opengl-sw "%~dp0release\Editor.exe" >nul
    ) else (
        echo [WARNING] windeployqt.exe not found at !QT_BIN!. Skipping Qt deployment.
    )

    :: External DLLs
    echo [INFO] Copying external DLLs...
    copy /Y "..\external-lib\libsndfile\libsndfile-1.2.2-win64\bin\sndfile.dll" "release\" >nul
    copy /Y "..\external-lib\fftw\libfftw3-3.dll" "release\" >nul
    
    echo [INFO] Deployment complete.
)
