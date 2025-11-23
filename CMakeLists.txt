cmake_minimum_required(VERSION 3.20.0)

# Set default board and build type if not specified
if(NOT DEFINED BOARD)
  set(BOARD nrf52833dongle)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(UWB_TAG_FIRMWARE)

target_sources(app PRIVATE 
    src/main.c
    src/uwb_driver.c
)
