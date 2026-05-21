#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Bootstrap the wows-depack Windows build environment.
.DESCRIPTION
    Installs MSYS2 (via winget), sets up the MinGW-w64 toolchain, bootstraps the
    bundled vcpkg, and installs the required build dependencies (zlib, pcre2, cunit).
    Run once from an elevated PowerShell prompt before building for the first time.
.EXAMPLE
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
    .\misc\windows-bootstrap.ps1
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

function Write-Step([string]$msg) {
    Write-Host "`n==> $msg" -ForegroundColor Cyan
}

function Assert-Command([string]$cmd) {
    if (-not (Get-Command $cmd -ErrorAction SilentlyContinue)) {
        Write-Error "'$cmd' not found on PATH. Ensure the step that installs it completed successfully."
    }
}

# Locate the repository root (parent of the misc/ directory).
$RepoRoot = Split-Path -Parent $PSScriptRoot
$VcpkgExe = Join-Path $RepoRoot "deps\vcpkg\vcpkg.exe"
$Msys2Root = "C:\msys64"
$Msys2Bash = Join-Path $Msys2Root "usr\bin\bash.exe"
$Msys2Bin  = Join-Path $Msys2Root "mingw64\bin"

# ---------------------------------------------------------------------------
# Step 1: Install MSYS2 via winget
# ---------------------------------------------------------------------------

Write-Step "Checking for MSYS2..."

if (Test-Path $Msys2Bash) {
    Write-Host "MSYS2 already installed at $Msys2Root — skipping."
} else {
    Assert-Command winget
    Write-Host "Installing MSYS2 via winget..."
    winget install --id MSYS2.MSYS2 --source winget --silent --accept-package-agreements --accept-source-agreements
    if (-not (Test-Path $Msys2Bash)) {
        Write-Error "MSYS2 installation failed or was installed to an unexpected location. Expected: $Msys2Root"
    }
}

# ---------------------------------------------------------------------------
# Step 2: Update MSYS2 and install MinGW-w64 toolchain
# ---------------------------------------------------------------------------

Write-Step "Updating MSYS2 package database and installing MinGW-w64 toolchain..."

# First pacman run may require a restart of the MSYS2 runtime; run twice to be safe.
$PacmanArgs = @(
    "--noconfirm", "-Syuu",
    "mingw-w64-x86_64-toolchain",
    "mingw-w64-x86_64-cmake",
    "git"
)

& $Msys2Bash -lc ("pacman " + ($PacmanArgs -join " "))
& $Msys2Bash -lc ("pacman " + ($PacmanArgs -join " "))

# ---------------------------------------------------------------------------
# Step 3: Add MinGW64 binaries to the current session's PATH
# ---------------------------------------------------------------------------

Write-Step "Ensuring MinGW64 bin directory is on PATH for this session..."

if ($Env:Path -notlike "*$Msys2Bin*") {
    $Env:Path = "$Msys2Bin;$Env:Path"
    Write-Host "Added $Msys2Bin to PATH (this session only)."
    Write-Host "To make it permanent, add it to your user or system PATH."
}

# ---------------------------------------------------------------------------
# Step 4: Bootstrap vcpkg
# ---------------------------------------------------------------------------

Write-Step "Bootstrapping vcpkg..."

$BootstrapScript = Join-Path $RepoRoot "deps\vcpkg\bootstrap-vcpkg.bat"
if (-not (Test-Path $BootstrapScript)) {
    Write-Error "vcpkg bootstrap script not found at '$BootstrapScript'. Make sure the submodule is initialised: git submodule update --init deps/vcpkg"
}

if (Test-Path $VcpkgExe) {
    Write-Host "vcpkg already bootstrapped — skipping."
} else {
    Push-Location (Join-Path $RepoRoot "deps\vcpkg")
    & .\bootstrap-vcpkg.bat -disableMetrics
    Pop-Location
    if (-not (Test-Path $VcpkgExe)) {
        Write-Error "vcpkg bootstrap failed."
    }
}

# ---------------------------------------------------------------------------
# Step 5: Install build dependencies via vcpkg
# ---------------------------------------------------------------------------

Write-Step "Installing vcpkg dependencies (zlib, pcre2, cunit) for x64-mingw-static..."

$OverlayTriplets = Join-Path $RepoRoot "deps\vcpkg\triplets\community"

& $VcpkgExe install --classic `
    "zlib:x64-mingw-static" `
    "pcre2:x64-mingw-static" `
    "cunit:x64-mingw-static" `
    "--overlay-triplets=$OverlayTriplets"

if ($LASTEXITCODE -ne 0) {
    Write-Error "vcpkg install failed."
}

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------

Write-Host ""
Write-Host "Bootstrap complete." -ForegroundColor Green
Write-Host ""
Write-Host "Open the MSYS2 MinGW64 shell and follow the build instructions in README.md."
