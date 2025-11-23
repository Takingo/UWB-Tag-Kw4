# Capture RTT output from running device
$JLinkExe = "C:\Program Files\SEGGER\JLink_V882\jlink.exe"

if (-not (Test-Path $JLinkExe)) {
    Write-Error "JLink.exe not found at $JLinkExe"
    exit 1
}

Write-Host "Connecting to device and capturing RTT output for 5 seconds..." -ForegroundColor Cyan
Write-Host "Press Ctrl+C if you see the output you need" -ForegroundColor Yellow

# Create temporary command file
$cmdContent = @"
device nrf52833
si swd
speed 4000
connect
r
g
Sleep 5000
exit
"@

$cmdFile = "temp_rtt_capture.jlink"
$cmdContent | Out-File -FilePath $cmdFile -Encoding ASCII

# Run JLink and capture output
& $JLinkExe -CommandFile $cmdFile

# Clean up
Remove-Item $cmdFile -ErrorAction SilentlyContinue

Write-Host "`nRTT capture complete!" -ForegroundColor Green
