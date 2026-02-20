# GitHub Actions Build & Release Guide

## Overview

This repository now has two GitHub Actions workflows:

1. **`build.yml`** - Original workflow (now disabled for automatic triggers)
2. **`build-release.yml`** - New manual workflow for creating releases

## Quick Start

### Step 1: Push Your Changes to GitHub

```bash
cd C:\Learn\equalizerAPO64
git add .
git commit -m "feat: VST Loopback Capture feature"
git push origin main
```

### Step 2: Trigger the Build Workflow

1. Go to your GitHub repository: `https://github.com/vunguyen22271/equalizerAPO64`
2. Click on **"Actions"** tab
3. Click on **"Build & Release (Fork)"** in the left sidebar
4. Click **"Run workflow"** button (top right)
5. Configure the build:
   - **Build variant**: Choose which variant to build
     - `x64-avx2` - Fastest build, works on all modern CPUs (Recommended for testing)
     - `x64-avx512` - High performance, requires AVX-512 support
     - `x64-avx10_1` - Bleeding edge, requires AVX-10.1 support
     - `arm64` - For ARM64 devices
     - `all` - Build all variants (takes ~60-90 minutes)
   - **Create GitHub Release**:
     - `false` - Just build artifacts (for testing)
     - `true` - Create a public GitHub Release
   - **Release tag**: (Only if creating release) e.g., `v1.4.2-vst-loopback`

6. Click **"Run workflow"**

### Step 3: Monitor the Build

The workflow will:
1. ✅ **Build** (~20-30 minutes per variant)
   - Compile EqualizerAPO core
   - Build Qt applications (Editor, DeviceSelector, UpdateChecker)
   - Package binaries

2. ✅ **Create Installer** (~5 minutes)
   - Generate NSIS installer executable

3. ✅ **Create Release** (Optional, ~2 minutes)
   - Upload installers to GitHub Releases
   - Generate release notes

### Step 4: Download Your Artifacts

#### Option A: Download from Actions (for testing)

1. Click on the completed workflow run
2. Scroll down to **"Artifacts"** section
3. Download the installers:
   - `EqualizerAPO_Setup-x64-avx2.exe`
   - `EqualizerAPO_Setup-x64-avx512.exe`
   - etc.

#### Option B: Download from Releases (for sharing)

1. Go to **"Releases"** tab
2. Click on your release (e.g., `v1.4.2-vst-loopback`)
3. Download the installer from **"Assets"**

## Usage Scenarios

### Scenario 1: Quick Test Build

**Goal**: Build just one variant to test if changes work

**Steps**:
1. Run workflow
2. Build variant: `x64-avx2`
3. Create GitHub Release: `false`
4. Wait ~25 minutes
5. Download artifact from Actions

### Scenario 2: Create Full Release for Sharing

**Goal**: Build all variants and create a public release

**Steps**:
1. Update `version.h` if needed:
   ```cpp
   #define MAJOR 1
   #define MINOR 4
   #define REVISION 3  // Bump this
   ```
2. Commit and push
3. Run workflow
4. Build variant: `all`
5. Create GitHub Release: `true`
6. Release tag: `v1.4.3-vst-loopback`
7. Wait ~90 minutes
8. Share the GitHub Release link with your friend

### Scenario 3: Build for Specific CPU

**Goal**: Build optimized version for a specific CPU

**Steps**:
1. Run workflow
2. Build variant: Choose based on CPU:
   - Intel 10th gen+ / AMD Zen 3+ → `x64-avx2`
   - Intel Ice Lake+ / AMD Zen 4+ → `x64-avx512`
   - Latest Intel Raptor Lake+ → `x64-avx10_1`
3. Create GitHub Release: `false`
4. Download and test

## Troubleshooting

### Build Fails on Dependency Download

**Problem**: Cannot download FFTW/muParserX/libsndfile artifacts

**Solution**:
- The workflow downloads from `TheFireKahuna`'s repositories
- These artifacts must be publicly accessible
- If they're not available, you may need to:
  1. Fork those dependency repositories
  2. Build them on your account
  3. Update the workflow to point to your artifacts

### Build Succeeds But Installer Fails

**Problem**: NSIS cannot find files

**Solution**:
- Check the "Prepare installer files" step logs
- Ensure all binaries were copied correctly
- Verify the NSIS script paths in `Setup/Setup64.nsi` or `Setup/SetupARM64.nsi`

### Workflow Doesn't Appear

**Problem**: Can't find "Build & Release (Fork)" in Actions

**Solution**:
1. Ensure you've pushed the `.github/workflows/build-release.yml` file
2. Wait ~30 seconds for GitHub to detect it
3. Refresh the Actions page

### Release Creation Fails

**Problem**: "Create Release" step fails with permission error

**Solution**:
1. Go to **Settings** → **Actions** → **General**
2. Scroll to **"Workflow permissions"**
3. Select **"Read and write permissions"**
4. Check **"Allow GitHub Actions to create and approve pull requests"**
5. Click **"Save"**

## Advanced: Customizing the Workflow

### Build Only x64-avx2 by Default

Edit `.github/workflows/build-release.yml`:

```yaml
inputs:
  build_variant:
    description: 'Build variant to create'
    required: true
    default: 'x64-avx2'  # ← Already set to this
```

### Add Pre-release Tag

In the "Create Release" step, change:

```yaml
prerelease: false  # Change to true for pre-release
```

### Customize Release Notes

Edit the `body` section in the "Create Release" step:

```yaml
body: |
  # Your custom release notes here

  ## New Features
  - Feature 1
  - Feature 2
```

## Cost & Quotas

### GitHub Actions Minutes

- **Free tier**: 2,000 minutes/month for private repos (unlimited for public)
- **Cost per build**:
  - Single variant: ~25 minutes
  - All variants: ~90 minutes
- **Recommendation**: Use `x64-avx2` for testing, `all` only for releases

### Artifact Storage

- **Free tier**: 500 MB storage, 1 GB bandwidth/month for private repos
- **Artifact size**: ~50-100 MB per variant
- **Retention**: 90 days (configurable in workflow)

## Best Practices

1. ✅ **Test locally first**: Use `vu-build-editor.bat` before pushing
2. ✅ **Use x64-avx2 for testing**: Fastest build time
3. ✅ **Create releases sparingly**: Use artifacts for day-to-day testing
4. ✅ **Semantic versioning**: Use `v{major}.{minor}.{patch}-{feature}` format
5. ✅ **Commit before release**: Tag meaningful commits, not WIP commits

## Further Reading

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [NSIS Documentation](https://nsis.sourceforge.io/Docs/)
- [Semantic Versioning](https://semver.org/)

---

**Last Updated**: 2026-02-20
**Workflow Version**: 1.0
