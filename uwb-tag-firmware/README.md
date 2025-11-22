# UWB TAG FIRMWARE

Ultra-Wideband (UWB) TX-mode tag firmware for Nordic Semiconductor nRF5833 using Zephyr RTOS and NRF Connect SDK v2.7.0.

## Features

- **TX-only Mode**: Transmits UWB BLINK frames every 500ms
- **Zephyr RTOS**: Lightweight real-time operating system
- **RTT Logging**: Real-time logging via SEGGER RTT for debugging
- **Minimal Footprint**: 39.5 KB flash usage (7.53%), 8.2 KB RAM (6.30%)

## Hardware

- **Board**: nRF52833 DK (nrf52833dk_nrf52833)
- **Processor**: ARM Cortex-M4
- **Flash**: 512 KB
- **RAM**: 128 KB

## Project Structure

```
uwb-tag-firmware/
├── src/
│   ├── main.c              # Application entry point
│   └── uwb_driver.c        # UWB transceiver driver (stub)
├── boards/
│   ├── nrf52833dk_nrf52833.dts     # Device tree
│   └── nrf52833dk_nrf52833.overlay # Board-specific overrides
├── CMakeLists.txt          # CMake build configuration
├── prj.conf                # Zephyr Kconfig settings
└── README.md               # This file
```

## Building

### Prerequisites
- NRF Connect SDK v2.7.0 or later
- Zephyr RTOS 3.7.99+
- ARM GCC 12.2.0+
- CMake 3.21.0+
- west (Zephyr package manager)

### Build Steps

```bash
cd uwb-tag-firmware
west build -b nrf52833dk_nrf52833
```

### Output

- Firmware ELF: `build/zephyr/zephyr.elf`
- Firmware HEX: `build/zephyr/zephyr.hex`
- Firmware BIN: `build/zephyr/zephyr.bin`

## Flashing

### Using pyocd

```powershell
# List connected devices
python -m pyocd list

# Flash firmware
python -m pyocd flash build/zephyr/zephyr.hex --target nrf52833 --probe 123456
```

### Using nRF Connect for Desktop

1. Open nRF Connect for Desktop
2. Select "Programmer"
3. Load `build/zephyr/zephyr.hex`
4. Click "Write"

## Serial Output - RTT

The firmware uses SEGGER RTT for logging:

1. Open nRF Connect for Desktop
2. Connect to the device
3. Open RTT terminal
4. View real-time debug output

**prj.conf settings:**
```conf
CONFIG_USE_SEGGER_RTT=y
CONFIG_LOG_BACKEND_RTT=y
CONFIG_RTT_CONSOLE=y
CONFIG_LOG_DEFAULT_LEVEL=4
```

## Configuration

Customize in `prj.conf`:

```conf
CONFIG_SERIAL=y              # Serial port
CONFIG_GPIO=y                # GPIO
CONFIG_SPI=y                 # SPI for UWB
CONFIG_LOG=y                 # Logging
CONFIG_LOG_DEFAULT_LEVEL=4   # Debug level
```

## Development Status

- ✅ Build system configured and working
- ✅ Firmware compiles without errors
- ✅ Successfully flashes to nRF52833
- ✅ RTT logging operational
- ⏳ DW3210 transceiver driver (in progress)
- ⏳ Actual UWB frame transmission (in progress)

## References

- [Zephyr RTOS Documentation](https://docs.zephyrproject.org/)
- [NRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/)
- [nRF52833 Datasheet](https://infocenter.nordicsemi.com/pdf/nRF52833_PS_v1.3.pdf)
