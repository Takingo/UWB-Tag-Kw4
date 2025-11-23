# GDB Debug Script for UWB Firmware
$JLinkGDB = "C:\Program Files\SEGGER\JLink_V882\JLinkGDBServerCL.exe"
$GDB = "C:\ncs\toolchains\2d382dcd92\opt\zephyr-sdk\arm-zephyr-eabi\bin\arm-zephyr-eabi-gdb.exe"
$ELF = "build\uwb-tag-firmware\zephyr\zephyr.elf"

Write-Host "Starting JLink GDB Server..." -ForegroundColor Cyan
$jlinkProcess = Start-Process -FilePath $JLinkGDB `
    -ArgumentList "-device", "NRF52833_XXAA", "-if", "SWD", "-speed", "4000", "-port", "2331" `
    -PassThru -WindowStyle Normal

Start-Sleep -Seconds 2

Write-Host "`nStarting GDB session..." -ForegroundColor Green
Write-Host "Commands to use:" -ForegroundColor Yellow
Write-Host "  break uwb_driver_init" -ForegroundColor White
Write-Host "  continue" -ForegroundColor White
Write-Host "  print dev_id" -ForegroundColor White
Write-Host "  next (step)" -ForegroundColor White
Write-Host "  quit" -ForegroundColor White
Write-Host ""

& $GDB $ELF -ex "target remote localhost:2331" -ex "monitor reset" -ex "load" -ex "break uwb_driver_init"

# Cleanup
$jlinkProcess | Stop-Process -Force -ErrorAction SilentlyContinue
