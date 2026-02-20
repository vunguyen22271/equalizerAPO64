# GitHub Actions Workflow Setup for Forks

This document explains how to set up the GitHub Actions workflow for building EqualizerAPO in your fork.

## Problem

The build workflow downloads pre-built dependencies from external repositories:
- `TheFireKahuna/amd-fftw`
- `TheFireKahuna/muparserx`
- `TheFireKahuna/libsndfile`
- `TheFireKahuna/tclap`

The default `GITHUB_TOKEN` in GitHub Actions cannot access artifacts from external repositories, which causes build failures in forks.

## Solution Options

Choose one of the following options to make the workflow work in your fork:

### Option 1: Use a Personal Access Token (Recommended for Quick Setup)

1. Create a Personal Access Token (PAT):
   - Go to GitHub Settings → Developer settings → Personal access tokens → Tokens (classic)
   - Click "Generate new token (classic)"
   - Give it a descriptive name (e.g., "EqualizerAPO Dependencies")
   - Select scopes:
     - `repo` (Full control of private repositories)
     - `workflow` (Update GitHub Action workflows)
   - Click "Generate token" and copy the token

2. Add the token to your repository secrets:
   - Go to your fork's Settings → Secrets and variables → Actions
   - Click "New repository secret"
   - Name: `DEPS_GITHUB_TOKEN`
   - Value: Paste your PAT token
   - Click "Add secret"

3. The workflow will now use this token to download artifacts from `TheFireKahuna`'s repositories

### Option 2: Fork All Dependencies (Recommended for Long-term Maintenance)

1. Fork the following repositories to your GitHub account:
   - https://github.com/TheFireKahuna/amd-fftw
   - https://github.com/TheFireKahuna/muparserx
   - https://github.com/TheFireKahuna/libsndfile
   - https://github.com/TheFireKahuna/tclap

2. Set up the repository variable:
   - Go to your fork's Settings → Secrets and variables → Actions → Variables tab
   - Click "New repository variable"
   - Name: `DEPS_REPO_OWNER`
   - Value: Your GitHub username
   - Click "Add variable"

3. Build each dependency repository:
   - Trigger the workflows in each forked repository to generate the required artifacts
   - Make sure all workflows complete successfully and produce the artifacts

4. The workflow will now download artifacts from your forked repositories

### Option 3: Use Environment Default (Upstream Repository Only)

If you're working on the upstream repository (`TheFireKahuna/equalizerAPO64`), no configuration is needed. The workflow defaults to using `TheFireKahuna` as the dependency repository owner.

## Verifying the Setup

After setting up one of the options above:

1. Push a commit or manually trigger the workflow
2. Check the "Actions" tab in your repository
3. All build jobs (x64-avx2, x64-avx512, x64-avx10_1, arm64) should complete successfully

## Troubleshooting

### Error: "Input required and not supplied: github_token"

This error occurs when:
- No PAT token is configured (Option 1), OR
- Dependencies are not forked to your account (Option 2), OR
- The `DEPS_REPO_OWNER` variable points to a repository without the required artifacts

**Solution**: Follow Option 1 or Option 2 above.

### Error: "Artifact not found"

This error occurs when the specified artifact doesn't exist in the dependency repository.

**Solution for Option 2**: 
- Make sure you've triggered the build workflows in all forked dependency repositories
- Verify that the artifacts are created with the correct names:
  - `fftw-windows-release-x64-avx2`, `fftw-windows-release-x64-avx512`, `fftw-windows-release-x64-avx10`, `fftw-windows-release-arm64`
  - `muparserx-msvc-release-x64-avx2`, `muparserx-msvc-release-x64-avx512`, `muparserx-msvc-release-x64-avx10`, `muparserx-msvc-release-ARM64`
  - `libsndfile-x64-avx2`, `libsndfile-x64-avx512`, `libsndfile-x64-avx10`, `libsndfile-arm64`

### Build succeeds but artifacts are missing

If the build completes but some Qt applications are not built:
- Check the "Build Qt Applications" step logs
- Look for warnings about vcvarsall.bat (these are usually harmless)
- Verify that all three Qt applications (Editor, DeviceSelector, UpdateChecker) are built successfully

## Additional Notes

- The default `DEPS_REPO_OWNER` in the workflow is set to `TheFireKahuna`
- Both repository variables and secrets are supported for `DEPS_REPO_OWNER` and `DEPS_GITHUB_TOKEN`
- Repository variables take precedence over environment variables
- Secrets take precedence over the default `GITHUB_TOKEN`

For more information, see the workflow file: `.github/workflows/build.yml`
