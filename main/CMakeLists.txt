set(libloragw_src
    "libloragw/loragw_ad5338r.c"
    "libloragw/loragw_aux.c"
    "libloragw/loragw_cal.c"
    "libloragw/loragw_com.c"
    "libloragw/loragw_debug.c"
    "libloragw/loragw_gpio.c"
    "libloragw/loragw_gps.c"
    "libloragw/loragw_hal.c"
    "libloragw/loragw_i2c.c"
    "libloragw/loragw_lbt.c"
    "libloragw/loragw_reg.c"
    "libloragw/loragw_spi.c"
    "libloragw/loragw_stts751.c"
    "libloragw/loragw_sx1250.c"
    "libloragw/loragw_sx125x.c"
    "libloragw/loragw_sx1261.c"
    "libloragw/loragw_sx1302.c"
    "libloragw/loragw_sx1302_rx.c"
    "libloragw/loragw_sx1302_timestamp.c"
    "libloragw/loragw_usb.c"
    "libloragw/sx1250_com.c"
    "libloragw/sx1250_spi.c"
    "libloragw/sx1250_usb.c"
    "libloragw/sx125x_com.c"
    "libloragw/sx125x_spi.c"
    "libloragw/sx1261_com.c"
    "libloragw/sx1261_spi.c"
    "libloragw/sx1261_usb.c"
)

set(libtools_src
    "libtools/base64.c"
    "libtools/parson.c"
    "libtools/tinymt32.c"
)

if(CONFIG_LIBLORAGW_TEST)
    set(libloragw_test_src
        "libloragw-test/test_loragw_toa.c"
        "libloragw-test/test_loragw_hal_tx.c"
        "libloragw-test/test_loragw_hal_rx.c"
        "libloragw-test/cli4test.c"
    )
    set(pkt_fwd_src "")
else()
    set(libloragw_test_src "")
    set(pkt_fwd_src
	"packet_forwarder/jitqueue.c"
	"packet_forwarder/lora_pkt_fwd.c"
        "packet_forwarder/ioe.c"
        "packet_forwarder/led_indication.c"
        "packet_forwarder/web_config.c"
        "packet_forwarder/http_server.c"
    )
endif()

# Build main/test/*.c conditionally based on Kconfig options defined in main/Kconfig
set(main_src "")

if(CONFIG_TEST_ORIGINAL_SPI_TEST)
    list(APPEND main_src "test/original_spi_test.c")
endif()

if(CONFIG_TEST_SPI_MAIN)
    list(APPEND main_src "test/spi_main.c")
endif()

if(CONFIG_TEST_LORAGW_CAL)
    list(APPEND main_src "test/test_loragw_cal.c")
endif()

if(CONFIG_TEST_LORAGW_CAPTURE_RAM)
    list(APPEND main_src "test/test_loragw_capture_ram.c")
endif()

if(CONFIG_TEST_LORAGW_COUNTER)
    list(APPEND main_src "test/test_loragw_counter.c")
endif()

if(CONFIG_TEST_LORAGW_GPS)
    list(APPEND main_src "test/test_loragw_gps.c")
endif()

if(CONFIG_TEST_LORAGW_HAL_RX)
    list(APPEND main_src "test/test_loragw_hal_rx.c")
endif()

if(CONFIG_TEST_LORAGW_HAL_TX)
    list(APPEND main_src "test/test_loragw_hal_tx.c")
endif()

if(CONFIG_TEST_LORAGW_I2C)
    list(APPEND main_src "test/test_loragw_i2c.c")
endif()

if(CONFIG_TEST_LORAGW_REG)
    list(APPEND main_src "test/test_loragw_reg.c")
endif()

if(CONFIG_TEST_LORAGW_SPI)
    list(APPEND main_src "test/test_loragw_spi.c")
endif()

if(CONFIG_TEST_LORAGW_SPI_SX1250)
    list(APPEND main_src "test/test_loragw_spi_sx1250.c")
endif()

if(CONFIG_TEST_NETWORK_CONNECTION)
    list(APPEND main_src "test/test_network_connection.c")
endif()

# If any test is enabled, disable the packet forwarder main to avoid app_main conflicts
if(CONFIG_TEST_ORIGINAL_SPI_TEST OR CONFIG_TEST_SPI_MAIN OR CONFIG_TEST_LORAGW_CAL OR 
   CONFIG_TEST_LORAGW_CAPTURE_RAM OR CONFIG_TEST_LORAGW_COUNTER OR CONFIG_TEST_LORAGW_GPS OR 
   CONFIG_TEST_LORAGW_HAL_RX OR CONFIG_TEST_LORAGW_HAL_TX OR CONFIG_TEST_LORAGW_I2C OR 
   CONFIG_TEST_LORAGW_REG OR CONFIG_TEST_LORAGW_SPI OR CONFIG_TEST_LORAGW_SPI_SX1250 OR 
   CONFIG_TEST_NETWORK_CONNECTION)
    set(pkt_fwd_src "")
endif()

idf_component_register(SRCS
    "${libtools_src}"
    "${libloragw_src}"
    "${libloragw_test_src}"
    "${pkt_fwd_src}"
    "${main_src}"
    INCLUDE_DIRS "libloragw"
    INCLUDE_DIRS "libtools"
    INCLUDE_DIRS "libloragw-test"
    INCLUDE_DIRS "packet_forwarder"
)
