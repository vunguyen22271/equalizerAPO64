# Quick Start: GitHub Actions for Your Fork

## ✅ What Was Done

### 1. Modified `.github/workflows/build.yml`
- **Disabled** automatic triggers (push/pull_request)
- **Kept** `workflow_dispatch` for manual triggering

### 2. Created `.github/workflows/build-release.yml`
- **New workflow** specifically for your fork
- **Manual trigger only** (`workflow_dispatch`)
- **Configurable build options**:
  - Choose specific variant or build all
  - Option to create GitHub Release
  - Custom release tags

### 3. Created `docs/GITHUB_ACTIONS_GUIDE.md`
- Complete guide on using the workflows
- Troubleshooting tips
- Usage scenarios

## 🚀 Next Steps

### Step 1: Commit and Push These Changes

```bash
cd C:\Learn\equalizerAPO64

# Stage all changes
git add .github/workflows/build.yml
git add .github/workflows/build-release.yml
git add docs/GITHUB_ACTIONS_GUIDE.md
git add QUICK_START_GITHUB_ACTIONS.md

# Commit
git commit -m "feat: add GitHub Actions workflow for fork releases"

# Push to your fork
git push origin main
```

### Step 2: Enable GitHub Actions Permissions

1. Go to: `https://github.com/vunguyen22271/equalizerAPO64/settings/actions`
2. Under **"Workflow permissions"**:
   - ✅ Select **"Read and write permissions"**
   - ✅ Check **"Allow GitHub Actions to create and approve pull requests"**
3. Click **"Save"**

### Step 3: Run Your First Build

1. Go to: `https://github.com/vunguyen22271/equalizerAPO64/actions`
2. Click **"Build & Release (Fork)"** in the left sidebar
3. Click **"Run workflow"** (green button, top right)
4. Configure:
   - **Build variant**: `x64-avx2` (for quick test)
   - **Create GitHub Release**: `false` (just build, don't release)
   - **Release tag**: (leave empty)
5. Click **"Run workflow"**
6. Wait ~25 minutes
7. Download installer from **"Artifacts"** section

## 📦 What Gets Built

The workflow will build:

### For Each Variant:
- ✅ `EqualizerAPO.dll` - Core audio processing
- ✅ `Editor.exe` - Configuration editor **with VST Loopback**
- ✅ `DeviceSelector.exe` - Device selection tool
- ✅ `UpdateChecker.exe` - Update checker
- ✅ `Benchmark.exe` - Performance benchmark
- ✅ `VoicemeeterClient.exe` - Voicemeeter integration
- ✅ All Qt DLLs and dependencies
- ✅ FFTW, libsndfile libraries

### Output:
- 📁 **Artifacts** (for testing): Downloadable from Actions page
- 📦 **Installer** (for distribution): `EqualizerAPO_Setup-{variant}.exe`
- 🏷️ **GitHub Release** (optional): Public release with all installers

## 🎯 Usage Examples

### Example 1: Quick Test (Recommended First Run)
```
Build variant: x64-avx2
Create release: false
Release tag: (empty)

Time: ~25 minutes
Output: Download from Actions → Artifacts
```

### Example 2: Share with Friend
```
Build variant: all
Create release: true
Release tag: v1.4.2-vst-loopback

Time: ~90 minutes
Output: Public release at https://github.com/vunguyen22271/equalizerAPO64/releases
```

### Example 3: Test Specific Optimization
```
Build variant: x64-avx512
Create release: false
Release tag: (empty)

Time: ~25 minutes
Output: Download from Actions → Artifacts
```

## 🔧 Important Notes

### Dependencies
The workflow downloads pre-built dependencies from TheFireKahuna's repositories:
- ✅ FFTW (Fast Fourier Transform library)
- ✅ muParserX (Math expression parser)
- ✅ libsndfile (Audio file I/O)
- ✅ TCLAP (Command-line argument parser)

**These must be publicly accessible** for the workflow to work. If you encounter download errors, these repositories might have restricted access.

### GitHub Actions Costs
- **Public repositories**: Unlimited minutes ✅
- **Private repositories**: 2,000 free minutes/month
- **Your repo**: Currently public, so **FREE**

### Build Matrix
Each variant builds independently in parallel:
- `x64-avx2` - Standard, works on all modern CPUs
- `x64-avx512` - High performance, Intel Ice Lake+, AMD Zen 4+
- `x64-avx10_1` - Bleeding edge, Intel Raptor Lake+
- `arm64` - ARM64 devices (Surface Pro X, etc.)

## 📝 Versioning

Current version: **v1.4.2** (from `version.h`)

To bump version for next release:
1. Edit `version.h`:
   ```cpp
   #define MAJOR 1
   #define MINOR 4
   #define REVISION 3  // Increment this
   ```
2. Commit: `git commit -am "chore: bump version to 1.4.3"`
3. Push: `git push`
4. Run workflow with release tag: `v1.4.3-vst-loopback`

## 🆘 Troubleshooting Quick Reference

| Problem | Solution |
|---------|----------|
| Workflow not showing | Push the workflow file, wait 30s, refresh |
| Permission denied (release) | Enable write permissions in repo settings |
| Dependency download fails | Check TheFireKahuna repos are accessible |
| Build succeeds, installer fails | Check NSIS script paths in Setup/*.nsi |
| Can't find artifacts | Look under workflow run → "Artifacts" section |

## 📚 More Information

See `docs/GITHUB_ACTIONS_GUIDE.md` for:
- Detailed troubleshooting
- Advanced customization
- Cost analysis
- Best practices

---

**Created**: 2026-02-20
**Your Repository**: https://github.com/vunguyen22271/equalizerAPO64
**Actions Page**: https://github.com/vunguyen22271/equalizerAPO64/actions
