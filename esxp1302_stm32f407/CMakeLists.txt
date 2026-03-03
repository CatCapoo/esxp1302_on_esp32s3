cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME esxp1302_stm32f407)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# ---------------------------------------------------------------------------
# LoRaGW library sources
# ---------------------------------------------------------------------------
set(LORAGW_SOURCES
    libloragw/loragw_spi.c
    libloragw/loragw_com.c
    libloragw/loragw_aux.c
    libloragw/loragw_reg.c
    libloragw/loragw_gpio.c
    libloragw/loragw_sx1250.c
    libloragw/loragw_sx1302.c
    libloragw/loragw_debug.c
    libloragw/sx1250_spi.c
    libloragw/sx1250_com.c
    libtools/tinymt32.c
)

# ---------------------------------------------------------------------------
# Test sources
# ---------------------------------------------------------------------------
set(TEST_SOURCES
    test/test_loragw_spi.c
    test/test_loragw_spi_sx1250.c
    test/test_loragw_reg.c
    test/test_loragw_i2c_oled.c
    test/test_loragw_i2c_lm75a.c
)

# ---------------------------------------------------------------------------
# LoRaGW I2C peripheral sources
# ---------------------------------------------------------------------------
set(I2C_SOURCES
    libloragw/loragw_i2c.c
    libloragw/loragw_lm75a.c
    libloragw/loragw_oled.c
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${LORAGW_SOURCES}
    ${I2C_SOURCES}
    ${TEST_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    libloragw
    libtools
    test
)

# ---------------------------------------------------------------------------
# Test selection  –  uncomment exactly ONE of the following lines
# ---------------------------------------------------------------------------
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # TEST_LORAGW_SPI             # SPI read/write stress test
    # TEST_LORAGW_SPI_SX1250      # SX1250 register R/W via SX1302
    # TEST_LORAGW_REG             # SX1302 register default-value & R/W test
    # TEST_LORAGW_I2C_OLED        # SSD1306 OLED on I2C2
    TEST_LORAGW_I2C_LM75A      # LM75A temperature sensor on I2C2
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
