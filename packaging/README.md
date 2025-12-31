# CCExtractor Packaging

This directory contains packaging configurations for Windows package managers.

## Windows Package Manager (winget)

### Initial Setup (One-time)

1. **Calculate MSI hash** for the current release:
   ```powershell
   certutil -hashfile CCExtractor.0.96.1.msi SHA256
   ```

2. **Update the manifest files** in `winget/` with the SHA256 hash

3. **Fork microsoft/winget-pkgs** to the CCExtractor organization:
   - Go to https://github.com/microsoft/winget-pkgs
   - Fork to https://github.com/CCExtractor/winget-pkgs

4. **Submit initial manifest** via PR:
   - Clone your fork
   - Create directory: `manifests/c/CCExtractor/CCExtractor/0.96.1/`
   - Copy the three YAML files from `winget/`
   - Submit PR to microsoft/winget-pkgs

5. **Create GitHub token** for automation:
   - Go to GitHub Settings > Developer settings > Personal access tokens > Tokens (classic)
   - Create token with `public_repo` scope
   - Add as secret `WINGET_TOKEN` in CCExtractor/ccextractor repository

### Automated Updates

After the initial submission is merged, the `publish_winget.yml` workflow will automatically submit PRs for new releases.

## Chocolatey

### Initial Setup (One-time)

1. **Create Chocolatey account**:
   - Register at https://community.chocolatey.org/account/Register

2. **Get API key**:
   - Go to https://community.chocolatey.org/account
   - Copy your API key

3. **Add secret**:
   - Add `CHOCOLATEY_API_KEY` secret to CCExtractor/ccextractor repository

### Package Structure

```
chocolatey/
├── ccextractor.nuspec      # Package metadata
└── tools/
    ├── chocolateyInstall.ps1    # Installation script
    └── chocolateyUninstall.ps1  # Uninstallation script
```

### Manual Testing

```powershell
cd packaging/chocolatey

# Update version and checksum in files first, then:
choco pack ccextractor.nuspec

# Test locally
choco install ccextractor --source="'.'" --yes --force

# Verify
ccextractor --version
```

### Automated Updates

The `publish_chocolatey.yml` workflow automatically:
1. Downloads the MSI from the release
2. Calculates the SHA256 checksum
3. Updates the nuspec and install script
4. Builds and tests the package
5. Pushes to Chocolatey

Note: Chocolatey packages go through moderation before being publicly available.

## Workflow Triggers

Both workflows trigger on:
- **Release published**: Automatic publishing when a new release is created
- **Manual dispatch**: Can be triggered manually with a specific tag

## Secrets Required

| Secret | Purpose |
|--------|---------|
| `WINGET_TOKEN` | GitHub PAT with `public_repo` scope for winget PRs |
| `CHOCOLATEY_API_KEY` | Chocolatey API key for package uploads |
