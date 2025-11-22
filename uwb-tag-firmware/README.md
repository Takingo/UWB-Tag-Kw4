# UWB TAG FIRMWARE

Ultra-Wideband (UWB) tag firmware for Nordic Semiconductor nRF devices using NRF Connect SDK.

## Project Structure

- `src/` - Application source code
- `boards/` - Board-specific device tree and configurations
- `config/` - Application configuration files
- `CMakeLists.txt` - CMake build configuration
- `prj.conf` - Zephyr project configuration

## Building

### Prerequisites
- NRF Connect for Desktop installed
- Zephyr SDK configured
- Compatible nRF SoC (nRF5340, nRF5240, etc.)

### Build Steps
1. Open project in NRF Connect for Desktop
2. Select your target board
3. Build the project
4. Flash to device

## Configuration

Configuration options can be adjusted in `prj.conf` for your specific use case.

## Development

Main application code is located in `src/main.c`.
