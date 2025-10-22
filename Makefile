# Arduino CLI Makefile for foamer-display
# ESP32-S3 MatrixPortal board

BOARD_FQBN = esp32:esp32:adafruit_matrixportal_esp32s3
PORT ?= $(shell arduino-cli board list | grep -i "esp32" | head -n1 | awk '{print $$1}')

.PHONY: compile upload

compile:
	arduino-cli compile --log --fqbn $(BOARD_FQBN) firmware/foamer-display

upload: compile
	arduino-cli upload --log --fqbn $(BOARD_FQBN) --port $(PORT) firmware/foamer-display
