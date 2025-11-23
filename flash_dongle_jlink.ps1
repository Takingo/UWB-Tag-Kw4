$JLinkExe = "C:\Program Files\SEGGER\JLink_V882\jlink.exe"
$Device = "nrf52833"
$HexFile = Resolve-Path "build\merged.hex"
$CmdFile = "flash_cmd.jlink"

if (-not (Test-Path $JLinkExe)) {
    Write-Error "JLink.exe not found at $JLinkExe"
    exit 1
}

if (-not (Test-Path $HexFile)) {
    Write-Error "Hex file not found at $HexFile"
    exit 1
}

$ScriptContent = @"
device $Device
si swd
speed 4000
loadfile $HexFile
r
g
exit
"@

Set-Content -Path $CmdFile -Value $ScriptContent -Encoding ASCII

Write-Host "Flashing $HexFile to $Device using JLink..." -ForegroundColor Cyan
& $JLinkExe -CommandFile $CmdFile

if ($LASTEXITCODE -eq 0) {
    Write-Host "Flashing complete!" -ForegroundColor Green
} else {
    Write-Error "Flashing failed with exit code $LASTEXITCODE"
}
