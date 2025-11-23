# UWB Tag Firmware - Build Instructions

## Prerequisites

1. Nordic nRF Connect SDK v2.7.0 installed
2. GNU ARM Embedded Toolchain
3. west build tool configured
4. Visual Studio Code with Zephyr extension (optional)

## Building the Firmware

### Using west (Recommended)

```bash
west build -b nrf52833dk_nrf52833 -d build
```

### Clean Build

```bash
west build -b nrf52833dk_nrf52833 -d build --pristine=always
```

### Flashing to Device

```bash
west flash -d build
```

## VS Code Integration

### Using Tasks

Press `Ctrl+Shift+B` to run the default build task, or select from the task menu:
- **Build UWB Tag Firmware** - Standard build
- **Clean Build** - Full rebuild from scratch
- **Flash to Device** - Build and flash to connected device

### Debug

Press `F5` to start debugging (requires built firmware).

## Configuration

### Main Configuration File: `prj.conf`

Edit this file to enable/disable features:
- `CONFIG_GPIO` - GPIO support
- `CONFIG_SPI` - SPI interface for UWB transceiver
- `CONFIG_LOG` - Logging framework
- `CONFIG_UART_CONSOLE` - Serial console

### Board Specific

Board files are located in `boards/`:
- `nrf52833dk_nrf52833.dts` - Device tree source
- `nrf52833dk_nrf52833.overlay` - Device tree overlay
- `nrf52833dk_nrf52833_defconfig` - Default board configuration

## Project Structure

```
uwb-tag-firmware/
├── CMakeLists.txt              # CMake build configuration
├── CMakePresets.json           # CMake preset configurations
├── Kconfig                     # Kconfig menu
├── prj.conf                    # Zephyr project configuration
├── boards/                     # Board-specific files
│   ├── nrf52833dk_nrf52833.dts
│   ├── nrf52833dk_nrf52833.overlay
│   └── nrf52833dk_nrf52833_defconfig
├── src/                        # Application source code
│   ├── main.c                  # Main application entry point
│   └── uwb_driver.c            # UWB driver stub
└── .vscode/                    # VS Code configuration
    ├── tasks.json              # Build tasks
    ├── launch.json             # Debug configuration
    └── settings.json           # VS Code settings
```

## Troubleshooting

### Build Fails with "command failed with return code: 1"

This typically indicates a configuration issue:
1. Ensure `prj.conf` has valid Zephyr configuration options
2. Check that `Kconfig` properly defines configuration structure
3. Run a clean build: `west build -b nrf52833dk_nrf52833 -d build --pristine=always`

### Missing Dependencies

Ensure the following are installed:
- `west`: NRF Connect build tool
- `arm-none-eabi-gcc`: ARM compiler
- `ninja`: Build system
- `python3`: Required by Zephyr build system

Install missing tools:
```bash
pip install -r $ZEPHYR_BASE/../requirements.txt
```

## Next Steps

1. Implement UWB driver for DW3210 transceiver
2. Add SPI communication interface
3. Configure DW3210 registers for positioning
4. Implement tag positioning logic
5. Add real-time data logging

## References

- [Zephyr Documentation](https://docs.zephyrproject.org/)
- [NRF Connect SDK Documentation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/)
- [DW3210 Datasheet](https://www.decawave.com/)
