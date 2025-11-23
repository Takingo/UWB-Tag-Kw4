# UWB Tag Firmware - Pin Configuration Status

## Current Status: ⚠️ SPI Communication FAILING

### Working Configuration (Confirmed):
- ✅ **Software logic**: LED blinks slowly (500ms) with fake Device ID
- ✅ **nRF52833**: Boots correctly, GPIO works, SPI driver loads
- ✅ **Reset sequence**: Hardware reset working (P0.29)
- ✅ **IRQ pin**: Configured as INPUT (P0.03)

### Failing Configuration:
- ❌ **Real SPI communication**: Device ID reads 0x00000000
- ❌ **CS pin**: Currently P0.15, but DW3110 not responding
- ❌ **Possible**: MISO/MOSI/CLK pins also incorrect

## Pin Mapping Attempts:

### Attempt 1: Initial guess
- CS=P0.03, MISO=P0.28, MOSI=P0.30, CLK=P0.31, IRQ=P0.11
- Result: ❌ Failed

### Attempt 2: From text schematic
- CS=P0.02, MISO=P0.28, MOSI=P0.30, CLK=P0.31, IRQ=P0.11  
- Result: ❌ Failed

### Attempt 3: IRQ fix
- CS=P0.02, MISO=P0.28, MOSI=P0.30, CLK=P0.31, IRQ=P0.03
- Result: ❌ Failed

### Attempt 4: From full schematic
- CS=P0.15, MISO=P0.28, MOSI=P0.30, CLK=P0.31, IRQ=P0.03
- Result: ❌ Still failing (current state)

## Next Steps:

1. **Verify CS pin with multimeter/logic analyzer**
   - Check which GPIO toggles during SPI transaction
   
2. **Try alternative CS pins systematically:**
   - P0.02 (old attempt)
   - P0.04 
   - P0.05
   - P0.06
   - P0.11
   - P0.17

3. **Verify MISO/MOSI/CLK pins**
   - Current: CLK=P0.31, MOSI=P0.30, MISO=P0.28
   - May need to swap MISO/MOSI

4. **Check for pin conflicts:**
   - P0.15 might be used for LIS2DH INT1/INT2
   - I2C pins (P0.09/P0.10) might conflict with NFC

## Schematic Analysis:
- DW3110TR13 chip clearly visible in top-right
- nRF52833 in bottom-left
- Pin traces hard to follow without high-res image
- Need to trace SPICSN signal from DW3110 to nRF GPIO

## Debug Evidence:
- `spi_transceive_dt()` returns 0 (success)
- RX buffer contains 0x00 0x00 0x00 0x00 0x00
- MISO stuck LOW or chip not selected
- Expected: 0x30 0x01 0xCA 0xDE (Device ID 0xDECA0130)
