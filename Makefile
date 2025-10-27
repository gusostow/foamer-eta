# PlatformIO Makefile for foamer-display
# ESP32-S3 MatrixPortal board

PROJECT_DIR = firmware/foamer-display
PIO = platformio

.PHONY: compile upload monitor clean compiledb

compile:
	cd $(PROJECT_DIR) && $(PIO) run

upload:
	cd $(PROJECT_DIR) && $(PIO) run -t upload

monitor:
	cd $(PROJECT_DIR) && $(PIO) device monitor

upload-monitor:
	cd $(PROJECT_DIR) && $(PIO) run -t upload -t monitor

clean:
	cd $(PROJECT_DIR) && $(PIO) run -t clean

compiledb:
	cd $(PROJECT_DIR) && $(PIO) run -t compiledb
