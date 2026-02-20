# GitHub Actions Workflow Setup for Forks

This document explains how to set up the GitHub Actions workflow for building EqualizerAPO in your fork.

## Problem

The build workflow downloads pre-built dependencies from external repositories:
- `TheFireKahuna/amd-fftw`
- `TheFireKahuna/muparserx`
- `TheFireKahuna/libsndfile`
- `TheFireKahuna/tclap`

These repositories are **public**, and their artifacts are accessible without authentication. The workflow is now configured to work in forks without any additional setup.

## How It Works

The workflow has been updated to:
1. Access public artifacts **without requiring authentication** (works for all forks by default)
2. Support configurable repository owner via `DEPS_REPO_OWNER` (for custom dependency sources)
3. Accept optional `DEPS_GITHUB_TOKEN` (only needed for private repos or rate limit issues)

## Default Behavior (No Setup Required)

**For most forks, no configuration is needed!** The workflow will:
- Download artifacts from TheFireKahuna's public repositories
- Use anonymous access (no authentication)
- Work for all platforms: x64-avx2, x64-avx512, x64-avx10_1, arm64

## Optional Configurations

### Option 1: Use Your Own Dependency Repositories

If you want to use your own forked dependencies:

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

### Option 2: Private Dependencies or Rate Limit Issues

If your dependencies are private or you encounter rate limit issues:

1. Create a Personal Access Token (PAT):
   - Go to GitHub Settings → Developer settings → Personal access tokens → Tokens (classic)
   - Click "Generate new token (classic)"
   - Give it a descriptive name (e.g., "EqualizerAPO Dependencies")
   - Select scopes:
     - `repo` (Full control of private repositories) - if dependencies are private
     - `public_repo` (Access public repositories) - if only accessing public repos
   - Click "Generate token" and copy the token

2. Add the token to your repository secrets:
   - Go to your fork's Settings → Secrets and variables → Actions
   - Click "New repository secret"
   - Name: `DEPS_GITHUB_TOKEN`
   - Value: Paste your PAT token
   - Click "Add secret"

3. The workflow will use this token for authentication

## Verifying the Setup

After pushing a commit or manually triggering the workflow:

1. Check the "Actions" tab in your repository
2. All build jobs (x64-avx2, x64-avx512, x64-avx10_1, arm64) should complete successfully

## Troubleshooting

### Error: "Artifact not found"

This error occurs when the specified artifact doesn't exist in the dependency repository.

**Common causes**:
- You set `DEPS_REPO_OWNER` to your username but didn't build the dependencies
- The artifact names in your forked repos don't match the expected names

**Solution**: 
- Either remove the `DEPS_REPO_OWNER` variable to use TheFireKahuna's public artifacts
- Or build all dependencies in your forked repositories with the correct artifact names:
  - `fftw-windows-release-x64-avx2`, `fftw-windows-release-x64-avx512`, `fftw-windows-release-x64-avx10`, `fftw-windows-release-arm64`
  - `muparserx-msvc-release-x64-avx2`, `muparserx-msvc-release-x64-avx512`, `muparserx-msvc-release-x64-avx10`, `muparserx-msvc-release-ARM64`
  - `libsndfile-x64-avx2`, `libsndfile-x64-avx512`, `libsndfile-x64-avx10`, `libsndfile-arm64`

### Rate Limit Issues

If you encounter rate limit errors:
- Add a `DEPS_GITHUB_TOKEN` secret with a PAT (see Option 2 above)
- This provides authenticated access with higher rate limits

### Build succeeds but artifacts are missing

If the build completes but some Qt applications are not built:
- Check the "Build Qt Applications" step logs
- Look for warnings about vcvarsall.bat (these are usually harmless)
- Verify that all three Qt applications (Editor, DeviceSelector, UpdateChecker) are built successfully

## Summary

**Default setup (recommended)**: No configuration needed! The workflow downloads public artifacts anonymously from TheFireKahuna's repositories.

**Custom setup**: Set `DEPS_REPO_OWNER` variable to use your own forked dependencies (requires building them first).

**Private repos**: Set `DEPS_GITHUB_TOKEN` secret with a PAT if your dependencies are private.

For more information, see the workflow file: `.github/workflows/build.yml`
