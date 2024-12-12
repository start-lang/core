
FQBN := esp32:esp32:m5stack_cardputer
LIBS := ../src ../lib/microcuts/src ../tests/unit ../lib/tools
INCLUDES := $(foreach d, $(LIBS), --library $d)
CFLAGS := --fqbn ${FQBN} ${INCLUDES} -j6 -v
REPEAT    := 100
BENCHMARK := -DSTOPFAIL -DBENCHMARK=${REPEAT} -DPRINT_TIMINGS -DCLOCKS_PER_SEC=1000
PORT := /dev/ttyACM0
BAUDRATE := 9600
UPLOAD := --upload -p ${PORT}
CC := time arduino-cli compile ${CFLAGS} ${UPLOAD}

ARDUINO_CLI_DEFINE_BUG := 1
ifeq (${ARDUINO_CLI_DEFINE_BUG},1)
OVERRIDDEN_FLAGS := -DARDUINO_HOST_OS=\"linux\" -DARDUINO_FQBN=\"esp32:esp32:m5stack_cardputer\" -DESP32=ESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=1 -DARDUINO_USB_MODE=1 -DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MSC_ON_BOOT=0 -DARDUINO_USB_DFU_ON_BOOT=0
BENCHMARK := --build-property build.extra_flags="${BENCHMARK} ${OVERRIDDEN_FLAGS}"
else
BENCHMARK := --build-property build.extra_flags="${BENCHMARK}"
endif

.PHONY: setup-arduino
setup-arduino:
	arduino-cli core update-index
	arduino-cli board listall arduino:avr
	arduino-cli board listall | grep arduino:avr

.PHONY: setup-esp32
setup-esp32:
	arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
	arduino-cli core install esp32:esp32
	arduino-cli board listall | grep esp32

.PHONY: setup-rp2040
setup-rp2040:
	arduino-cli config add board_manager.additional_urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
	arduino-cli core update-index
	arduino-cli core install rp2040:rp2040
	arduino-cli board listall | grep rp2040

.PHONY: setup-teensy
setup-teensy:
	arduino-cli config add board_manager.additional_urls https://www.pjrc.com/teensy/package_teensy_index.json
	arduino-cli core update-index
	arduino-cli core install teensy:avr
	arduino-cli board listall | grep teensy

.PHONY: setup-esp8

.PHONY: HelloWorld
HelloWorld:
	${CC} ${CFLAGS} HelloWorld


.PHONY: Tests
Tests:
	${CC} ${CFLAGS} Tests

.PHONY: benchmark
benchmark:
	${CC} ${CFLAGS} ${BENCHMARK} Tests
	python3 get_output.py ${PORT} ${BAUDRATE}



.PHONY: monitor
monitor:
	arduino-cli monitor -p ${PORT} --config ${BAUDRATE}