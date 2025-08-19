# Build script with optimizations
# Based on the original build-win64.ps1

param(
    [switch]$Debug,
    [switch]$InstallTools,
    [switch]$TrackMemory
)

$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH;

Write-Host "Building PSX Emulator with CDROM optimizations..." -ForegroundColor Green

# Help install required tools
if ($InstallTools) {
    Write-Host "`nInstalling required build tools..." -ForegroundColor Yellow
    
    # Check if chocolatey is available
    if (Get-Command choco -ErrorAction SilentlyContinue) {
        Write-Host "Installing tools via Chocolatey..." -ForegroundColor Cyan
        choco install mingw --confirm
        choco install make --confirm
    } 
    # Check if winget is available
    elseif (Get-Command winget -ErrorAction SilentlyContinue) {
        Write-Host "Installing tools via Winget..." -ForegroundColor Cyan
        winget install --id=MSYS2.MSYS2 --silent
        Write-Host "After MSYS2 installation, run: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make" -ForegroundColor Yellow
    }
    else {
        Write-Host "Please install one of the following:" -ForegroundColor Yellow
        Write-Host "1. Chocolatey: Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" -ForegroundColor Cyan
        Write-Host "2. MSYS2: https://www.msys2.org/" -ForegroundColor Cyan
        Write-Host "3. Visual Studio Build Tools: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Cyan
    }
    exit 0
}

# Get git information (same as original)
git fetch --all --tags

$VERSION_TAG = git describe --always --tags --abbrev=0
$COMMIT_HASH = git rev-parse --short HEAD
$OS_INFO = (Get-WMIObject win32_operatingsystem).caption + " " + `
           (Get-WMIObject win32_operatingsystem).version + " " + `
           (Get-WMIObject win32_operatingsystem).OSArchitecture

$SDL2_DIR = "SDL2-2.30.3\x86_64-w64-mingw32"
$PSX_DIR = "."

# Set optimization flags based on parameters
$OptFlags = "-Ofast -flto"

if ($Debug) {
    $OptFlags = "-O0 -g"
    Write-Host "Debug build enabled" -ForegroundColor Yellow
} else {
    $OptFlags += " -g"  # Keep debug symbols even in optimized builds
}

# Check if GCC is available
if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) {
    Write-Host "GCC compiler not found!" -ForegroundColor Red
    Write-Host "Please install MinGW-w64 or MSYS2" -ForegroundColor Yellow
    Write-Host "Run: .\build-win64.ps1 -InstallTools" -ForegroundColor Cyan
    exit 1
}

Write-Host "Using GCC compiler" -ForegroundColor Green

# Combine all compiler flags (based on original but with our optimizations)
$CFlags = "$OptFlags -march=native -mtune=native -ffast-math -funroll-loops"

# Add memory tracking flag if requested
if ($TrackMemory) {
    $CFlags += " -DENABLE_MEM_TRACKING"
    Write-Host "Memory tracking enabled" -ForegroundColor Yellow
}

Write-Host "Using compiler flags: $CFlags" -ForegroundColor Cyan

# Create bin directory
mkdir -Force -Path bin > $null

# Build the project (following original pattern exactly)
try {
    Write-Host "Building PSX emulator..." -ForegroundColor Green
    
    gcc -I"$($PSX_DIR)" `
        -I"$($PSX_DIR)\psx" `
        -I"$($SDL2_DIR)\include\SDL2" `
        "psx\*.c" `
        "psx\dev\*.c" `
        "psx\dev\cdrom\*.c" `
        "psx\input\*.c" `
        "frontend\*.c" `
        -o "bin\psxe.exe" `
        -DREP_VERSION="`"$($VERSION_TAG)`"" `
        -DREP_COMMIT_HASH="`"$($COMMIT_HASH)`"" `
        -DOS_INFO="`"$($OS_INFO)`"" `
        -L"$($SDL2_DIR)\lib" `
        -m64 -lSDL2main -lSDL2 -Wno-overflow `
        -Wall -pedantic -DLOG_USE_COLOR -DENABLE_MEM_TRACKING `
        -Wno-address-of-packed-member `
        $CFlags.Split(' ')
    
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed with exit code $LASTEXITCODE"
    }
    
    # Copy SDL2 DLL (same as original)
    Copy-Item -Path "$($SDL2_DIR)\bin\SDL2.dll" -Destination "bin"
    
    Write-Host "Build completed successfully!" -ForegroundColor Green
    
} catch {
    Write-Host "Build failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "`nTroubleshooting:" -ForegroundColor Yellow
    Write-Host "1. Install build tools: .\build-optimized.ps1 -InstallTools" -ForegroundColor Cyan
    Write-Host "2. Ensure SDL2 directory exists: $SDL2_DIR" -ForegroundColor Cyan
    Write-Host "3. Check that all source files exist" -ForegroundColor Cyan
    Write-Host "4. Try without optimizations first" -ForegroundColor Cyan
    exit 1
}

Write-Host "`nOptimization Summary:" -ForegroundColor Cyan
Write-Host "- Compiler: GCC"
Write-Host "- Optimization level: $OptFlags"
Write-Host "- Architecture tuning: Native"
Write-Host "- Fast math: Enabled"
Write-Host "- Loop unrolling: Enabled"
Write-Host "- Link-time optimization: $(if($OptFlags -like '*-flto*') { 'Enabled' } else { 'Disabled' })"

Write-Host "`nOutput files:" -ForegroundColor Green
Write-Host "- Executable: bin\psxe.exe"
Write-Host "- SDL2 Library: bin\SDL2.dll"
