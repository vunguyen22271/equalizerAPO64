# GitHub Actions Workflow Fixes Required

## Summary of Changes Between Original and Your Fork

Based on git diff between:
- **Original**: `7156020e9b68b72f64d2bf47b0c15f178078b378` (TheFireKahuna)
- **Your Fork**: `305e2c36ccdedabf0b9ae2a4ae7a9e9d616604fd` (vu.nguyen)

## Key Changes That Affect the Build Workflow

### 1. New Files Added
- ✅ `Editor/helpers/WASAPILoopback.cpp/.h` - WASAPI Loopback capture
- ✅ `external-lib/` directory - Local static libraries and dependencies
- ✅ `Setup/lib64/libfftw3-3.dll` - Runtime DLL

### 2. Dependency Structure Changes

#### Before (Original):
- Dependencies downloaded from TheFireKahuna's repos to `deps/`
- FFTW DLL: `libfftw3.dll`
- muParserX: Linked against pre-built `muparserx.lib`

#### After (Your Fork):
- Dependencies in repo at `external-lib/`
- FFTW DLL: `libfftw3-3.dll` ⚠️ **DIFFERENT FILENAME**
- muParserX: **Compiled from source** (39 .cpp files)

### 3. Editor.pro Configuration Changes

```diff
- LIBS += libfftw3.lib muparserx.lib
+ LIBS += libfftw3-3.lib
+ # muparserx.lib removed - now compiling from source

- Paths: F:/Git/... (absolute paths)
+ Paths: $$PWD/../external-lib/... (relative paths)

+ SOURCES += all muparserx .cpp files
```

### 4. NSIS Installer Script Changes

```diff
- File "${LIBPATH}\libfftw3.dll"
+ File "${LIBPATH}\libfftw3-3.dll"
```

## Critical Issues with Current Workflow

### ❌ Issue 1: Wrong DLL Filename

**Current Workflow Line 499**:
```powershell
Copy-Item ${{ github.workspace }}\deps\fftw\Release\libfftw3.dll
```

**Problem**: Your Editor.exe expects `libfftw3-3.dll`, not `libfftw3.dll`

**Fix Required**: Copy from `external-lib/` or rename the DLL

### ❌ Issue 2: Redundant Dependency Downloads

**Current Workflow**: Downloads FFTW, muParserX, libsndfile to `deps/`

**Your Build**: Uses `external-lib/` from repo

**Impact**:
- ✅ MSBuild projects (EqualizerAPO.dll, Benchmark.exe) use `deps/` → OK
- ✅ qmake projects (Editor.exe) use `external-lib/` → OK
- ⚠️ But packaging step copies from wrong location

### ❌ Issue 3: muParserX Handling

**Workflow**: Downloads pre-built muparserx.lib

**Your Code**: Compiles muParserX from source files in Editor.pro

**Impact**: The workflow downloads unnecessary muparserx artifacts, but qmake will compile from source anyway. This works, but wastes time.

## Required Workflow Fixes

### Fix 1: Update Package Binaries Step

**File**: `.github/workflows/build-release.yml`
**Lines**: ~499-500

**Change from**:
```powershell
Copy-Item ${{ github.workspace }}\deps\fftw\Release\libfftw3.dll -Destination $artifactPath -Force
Copy-Item ${{ github.workspace }}\deps\libsndfile\build\Release\sndfile.dll -Destination $artifactPath -Force
```

**Change to**:
```powershell
# Copy runtime DLLs from external-lib (which is in the repo)
# These match the filenames expected by the executables
Copy-Item ${{ github.workspace }}\external-lib\fftw\libfftw3-3.dll -Destination $artifactPath -Force
Copy-Item ${{ github.workspace }}\Setup\lib64\sndfile.dll -Destination $artifactPath -Force
```

### Fix 2: Update Installer Preparation Step

**File**: `.github/workflows/build-release.yml`
**Location**: `create-installer` job

**Ensure copying correct DLL names**:
```powershell
# The installer expects libfftw3-3.dll (not libfftw3.dll)
# Copy from external-lib which has the correct version
```

### Fix 3: Optional - Remove Unnecessary muParserX Download

**Rationale**: Since Editor.pro compiles muParserX from source, downloading pre-built muparserx artifacts is unnecessary for Qt builds.

**However**: Keep it if other MSBuild projects need it.

## Verification Checklist

After applying fixes, verify:

### ✅ Build Phase
- [ ] Common.vcxproj builds successfully
- [ ] EqualizerAPO.vcxproj builds successfully
- [ ] Editor.pro builds successfully (with muParserX compiled from source)
- [ ] All Qt apps build successfully

### ✅ Packaging Phase
- [ ] `libfftw3-3.dll` is copied to artifacts (not `libfftw3.dll`)
- [ ] `sndfile.dll` is copied to artifacts
- [ ] All Qt DLLs are copied
- [ ] Editor.exe is included

### ✅ Installer Phase
- [ ] NSIS finds `libfftw3-3.dll` in lib64/
- [ ] Installer includes all required DLLs
- [ ] Installer executable is created

### ✅ Runtime Test
- [ ] Installed Editor.exe launches without DLL errors
- [ ] VST Loopback Capture feature works
- [ ] WASAPI Loopback works

## Implementation Priority

### 🔴 Critical (Must Fix)
1. **Fix 1**: Change DLL copy commands to use correct filenames
2. Verify Setup/lib64 has libfftw3-3.dll before installer creation

### 🟡 Important (Should Fix)
3. Test the complete workflow with one variant first
4. Verify Qt builds find external-lib dependencies

### 🟢 Optional (Nice to Have)
5. Skip muParserX artifact download (not needed for Qt builds)
6. Add workflow input to skip dependency downloads if using external-lib

## Next Steps

1. ✅ Apply Fix 1 to build-release.yml
2. ✅ Commit and push changes
3. ✅ Run workflow with x64-avx2 variant (test build)
4. ✅ Check workflow logs for DLL-related errors
5. ✅ Download artifact and test Editor.exe locally
6. ✅ If successful, run full release build

---

**Created**: 2026-02-20
**Status**: Action Required
