# Quick RTT capture with timeout
$JLinkRTT = "C:\Program Files\SEGGER\JLink_V882\JLinkRTTLogger.exe"

Write-Host "Starting RTT capture for 3 seconds..." -ForegroundColor Cyan

# Start RTT logger in background
$process = Start-Process -FilePath $JLinkRTT `
    -ArgumentList "-Device", "NRF52833_XXAA", "-if", "SWD", "-Speed", "4000", "-RTTChannel", "0", "rtt_quick.txt" `
    -PassThru -WindowStyle Hidden

# Wait for RTT to connect and capture data
Start-Sleep -Seconds 3

# Kill the process
$process | Stop-Process -Force -ErrorAction SilentlyContinue

# Display captured data
Write-Host "`n=== RTT Output ===" -ForegroundColor Green
if (Test-Path "rtt_quick.txt") {
    Get-Content "rtt_quick.txt" | Select-Object -Skip 15 | Where-Object { $_ -notmatch "Transfer rate|Data written" }
} else {
    Write-Host "No RTT data captured" -ForegroundColor Red
}
