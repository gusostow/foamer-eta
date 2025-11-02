# PlatformIO Makefile for foamer-display
# ESP32-S3 MatrixPortal board

PROJECT_DIR = firmware/foamer-display
PIO = platformio

# Config profile to use (dev, prod, etc.)
# Pass PROFILE=xxx to make to select a profile
PROFILE ?= dev
export CONFIG_PROFILE = $(PROFILE)

.PHONY: compile upload monitor clean compiledb

compile:
	@echo "building with profile: $(PROFILE)"
	cd $(PROJECT_DIR) && $(PIO) run

upload:
	@echo "building with profile: $(PROFILE)"
	cd $(PROJECT_DIR) && $(PIO) run -t upload

monitor:
	cd $(PROJECT_DIR) && $(PIO) device monitor

upload-monitor:
	@echo "building with profile: $(PROFILE)"
	cd $(PROJECT_DIR) && $(PIO) run -t upload -t monitor

clean:
	cd $(PROJECT_DIR) && $(PIO) run -t clean

compiledb:
	cd $(PROJECT_DIR) && $(PIO) run -t compiledb
