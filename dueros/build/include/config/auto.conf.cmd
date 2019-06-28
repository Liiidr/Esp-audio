deps_config := \
	/home/zz/esp/esp-adf-mini/esp-idf/components/app_trace/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/aws_iot/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/bt/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/driver/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/esp32/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/esp_adc_cal/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/esp_event/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/esp_http_client/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/esp_http_server/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/ethernet/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/fatfs/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/freemodbus/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/freertos/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/heap/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/libsodium/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/log/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/lwip/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/mbedtls/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/mdns/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/mqtt/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/nvs_flash/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/openssl/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/pthread/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/spi_flash/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/spiffs/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/tcpip_adapter/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/vfs/Kconfig \
	/home/zz/esp/esp-adf-mini/esp-idf/components/wear_levelling/Kconfig \
	D:/GitHub/msys32/home/zz/esp/esp-adf-mini/components/audio_board/Kconfig.projbuild \
	/home/zz/esp/esp-adf-mini/esp-idf/components/bootloader/Kconfig.projbuild \
	D:/GitHub/msys32/home/zz/esp/esp-adf-mini/components/esp-adf-libs/Kconfig.projbuild \
	/home/zz/esp/esp-adf-mini/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/zz/esp/Esp-audio/dueros/main/Kconfig.projbuild \
	/home/zz/esp/esp-adf-mini/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/zz/esp/esp-adf-mini/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
