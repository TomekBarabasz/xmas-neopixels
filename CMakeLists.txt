# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)
set(COMPONENTS
    bootloader bootloader_support 
    efuse esp32 
    esp_common esp_hw_support esp_rom esp_system 
    esptool_py hal log main micro-ecc newlib partition_table soc spi_flash xtensa
    protocol_examples_common
    neopixels
)
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(xmas-neopixels)
