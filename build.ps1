# NRF Connect SDK Build Environment Setup
# This script sets up the environment for building UWB Tag Firmware

# Set NCS paths
$env:NCS_TOP = "C:\ncs\v2.7.0"
$env:ZEPHYR_BASE = "$env:NCS_TOP\zephyr"
$env:GNUARMEMB_TOOLCHAIN_PATH = "C:\ncs\toolchains\2d382dcd92\opt"

# Add toolchain to PATH
$env:PATH = "$env:GNUARMEMB_TOOLCHAIN_PATH\bin;" + $env:PATH

# Function to build the project
function Build-UWBFirmware {
    param(
        [string]$BuildType = "debug",
        [string]$Board = "nrf52833dongle",
        [switch]$Clean
    )
    
    $pythonPath = "$env:GNUARMEMB_TOOLCHAIN_PATH\bin\python.exe"
    
    if ($Clean) {
        Write-Host "Cleaning build directory..." -ForegroundColor Yellow
        Remove-Item -Path build -Recurse -Force -ErrorAction SilentlyContinue
    }
    
    Write-Host "Building UWB Tag Firmware for $Board..." -ForegroundColor Green
    & $pythonPath -m west build -b $Board -d build 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Build successful!" -ForegroundColor Green
        Write-Host "Firmware ELF: build\uwb-tag-firmware\zephyr\zephyr.elf" -ForegroundColor Cyan
        Write-Host "Firmware HEX: build\merged.hex" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Next step: Use Flash-UWBFirmware or follow FLASHING_GUIDE.txt" -ForegroundColor Yellow
    } else {
        Write-Host "✗ Build failed!" -ForegroundColor Red
        return $LASTEXITCODE
    }
}

# Function to flash the firmware
function Flash-UWBFirmware {
    param(
        [string]$Runner = "auto"
    )
    
    $pythonPath = "$env:GNUARMEMB_TOOLCHAIN_PATH\bin\python.exe"
    
    # Check for available flashing tools
    $hasNrfJprog = $null -ne (Get-Command nrfjprog.exe -ErrorAction SilentlyContinue)
    $hasJlink = $null -ne (Get-Command jlink.exe -ErrorAction SilentlyContinue)
    $hasPyocd = $null -ne (Get-Command pyocd -ErrorAction SilentlyContinue)
    
    if (-not $hasNrfJprog -and -not $hasJlink -and -not $hasPyocd) {
        Write-Host "⚠ No flashing tools found!" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Available options:" -ForegroundColor Yellow
        Write-Host "  1. Install nRF Command-Line Tools (includes nrfjprog)" -ForegroundColor Cyan
        Write-Host "     https://www.nordicsemi.com/Products/Development-tools/nRF-Command-Line-Tools"
        Write-Host ""
        Write-Host "  2. Use nRF Connect for Desktop with Programmer app" -ForegroundColor Cyan
        Write-Host "     https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-desktop"
        Write-Host "     Then select: build\merged.hex"
        Write-Host ""
        Write-Host "  3. Install J-Link tools and use:" -ForegroundColor Cyan
        Write-Host "     python -m west flash -r jlink -d build"
        Write-Host ""
        Write-Host "  4. See FLASHING_GUIDE.txt for more options" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "For now, the firmware HEX file is ready:" -ForegroundColor Green
        Write-Host "  build\merged.hex" -ForegroundColor Cyan
        return 1
    }
    
    Write-Host "Flashing firmware to device..." -ForegroundColor Green
    
    if ($Runner -eq "auto") {
        if ($hasNrfJprog) {
            Write-Host "Using nrfjprog..." -ForegroundColor Cyan
            & $pythonPath -m west flash -d build 2>&1
        } elseif ($hasJlink) {
            Write-Host "Using J-Link..." -ForegroundColor Cyan
            & $pythonPath -m west flash -r jlink -d build 2>&1
        } elseif ($hasPyocd) {
            Write-Host "Using pyocd..." -ForegroundColor Cyan
            & $pythonPath -m west flash -r pyocd -d build 2>&1
        }
    } else {
        & $pythonPath -m west flash -r $Runner -d build 2>&1
    }
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Flash successful!" -ForegroundColor Green
    } else {
        Write-Host "✗ Flash failed!" -ForegroundColor Red
        return $LASTEXITCODE
    }
}

# Function to generate flashing guide
function Show-FlashingGuide {
    $pythonPath = "$env:GNUARMEMB_TOOLCHAIN_PATH\bin\python.exe"
    Write-Host "Generating flashing guide..." -ForegroundColor Green
    & $pythonPath flash_helper.py
}

# Display setup information
Write-Host @"
╔════════════════════════════════════════════════════════════╗
║          UWB Tag Firmware Build Environment Setup          ║
╚════════════════════════════════════════════════════════════╝

Environment Variables:
  NCS_TOP:      $env:NCS_TOP
  ZEPHYR_BASE:  $env:ZEPHYR_BASE
  Toolchain:    $env:GNUARMEMB_TOOLCHAIN_PATH

Available Commands:
  Build-UWBFirmware         - Build the firmware
  Build-UWBFirmware -Clean  - Clean build
  Flash-UWBFirmware         - Flash to device (if tools installed)
  Show-FlashingGuide        - Display detailed flashing options

Example:
  Build-UWBFirmware
  Flash-UWBFirmware

"@ -ForegroundColor Cyan

