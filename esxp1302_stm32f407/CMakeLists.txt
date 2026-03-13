cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# --------------------------------------------------------------------------
# global_json.h: auto-generate from JSON configs before build
#
# Source files  : packet_forwarder/global_conf.json/*.json
# Generated file: packet_forwarder/global_json.h  (do not edit manually)
# Script        : scripts/gen_global_json.py
# --------------------------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GLOBAL_JSON_H   ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/global_json.h)
set(GEN_SCRIPT      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_global_json.py)
set(JSON_CONF_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/global_conf.json)

add_custom_command(
    OUTPUT  ${GLOBAL_JSON_H}
    COMMAND ${Python3_EXECUTABLE} ${GEN_SCRIPT}
    DEPENDS ${JSON_CONF_DIR}/global_conf.cn490.json
            ${JSON_CONF_DIR}/global_conf.eu868.json
            ${JSON_CONF_DIR}/global_conf.us915.json
            ${GEN_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating global_json.h from JSON configs"
    VERBATIM
)
add_custom_target(generate_global_json DEPENDS ${GLOBAL_JSON_H})

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


# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# global_json.h must exist before any source file that #includes it is compiled
add_dependencies(${CMAKE_PROJECT_NAME} generate_global_json)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# ---------------------------------------------------------------------------
# LoRaGW library sources (core)
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
    libloragw/gateway_config.c
    libloragw/gw_config_presets.c
    libloragw/uart_cli.c
    libtools/tinymt32.c
)

# ---------------------------------------------------------------------------
# LoRaGW HAL sources (needed by packet forwarder — lgw_start/stop/send/recv)
# ---------------------------------------------------------------------------
set(LORAGW_HAL_SOURCES
    libloragw/loragw_hal.c
    libloragw/loragw_cal.c
    libloragw/loragw_sx1302_rx.c
    libloragw/loragw_sx1302_timestamp.c
    libloragw/loragw_lbt.c
    libloragw/loragw_stts751.c
    libloragw/loragw_ad5338r.c
    libloragw/loragw_sx125x.c
    libloragw/loragw_sx1261.c
    libloragw/sx125x_spi.c
    libloragw/sx125x_com.c
    libloragw/sx1261_spi.c
    libloragw/sx1261_com.c
    libloragw/sx1261_usb.c
)

# ---------------------------------------------------------------------------
# Packet Forwarder sources
# ---------------------------------------------------------------------------
set(PKT_FWD_SOURCES
    packet_forwarder/lora_pkt_fwd.c
    packet_forwarder/jitqueue.c
    packet_forwarder/net_transport.c
    packet_forwarder/sntp_client.c
    libtools/parson.c
    libtools/base64.c
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
    test/test_w5500_udp.c
    test/test_config_cli.c
)

# ---------------------------------------------------------------------------
# LoRaGW I2C peripheral sources
# ---------------------------------------------------------------------------
set(I2C_SOURCES
    libloragw/loragw_i2c.c
    libloragw/loragw_lm75a.c
    libloragw/loragw_oled.c
)

# ---------------------------------------------------------------------------
# Core HAL and syscall sources
# ---------------------------------------------------------------------------
# NOTE: main.c, stm32f4xx_it.c, stm32f4xx_hal_msp.c, syscalls.c are already
#       included by cmake/stm32cubemx/CMakeLists.txt (INTERFACE library).
#       Only list files NOT present in that generated file.
set(CORE_SOURCES
    Core/Src/sntp_gettimeofday.c  # Overrides weak _gettimeofday in syscalls.c; keep here only
)

# ---------------------------------------------------------------------------
# W5500 Ethernet sources (WIZnet ioLibrary)
# ---------------------------------------------------------------------------
set(W5500_SOURCES
    W5500/Src/w5500.c
    W5500/Src/wizchip_conf.c
    W5500/Src/socket.c
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CORE_SOURCES}
    ${LORAGW_SOURCES}
    ${I2C_SOURCES}
    ${W5500_SOURCES}
    ${TEST_SOURCES}
    ${LORAGW_HAL_SOURCES}
    ${PKT_FWD_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    libloragw
    libtools
    test
    W5500/Inc
    packet_forwarder
)

# ---------------------------------------------------------------------------
# Build mode selection  –  uncomment exactly ONE of the following lines
# ---------------------------------------------------------------------------
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    VERSION_STRING="ESXP1302-v0.9.0"  # shown in banner: *** Packet Forwarder *** Version: ...
    # TEST_LORAGW_SPI             # SPI read/write stress test
    # TEST_LORAGW_SPI_SX1250      # SX1250 register R/W via SX1302
    # TEST_LORAGW_REG             # SX1302 register default-value & R/W test
    # TEST_LORAGW_I2C_OLED        # SSD1306 OLED on I2C2
    # TEST_LORAGW_I2C_LM75A      # LM75A temperature sensor on I2C2
    # TEST_W5500_UDP              # W5500 Ethernet + UDP echo test
    # TEST_CONFIG_CLI             # Flash config R/W + UART CLI validation
    PKT_FWD                     # Full packet forwarder mode
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
