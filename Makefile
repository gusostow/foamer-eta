# PlatformIO Makefile for foamer-display
# ESP32-S3 MatrixPortal board

PROJECT_DIR = firmware/foamer-display
PIO = platformio
ENV ?= dev

# Get local IPv4 address (filters out loopback)
LOCAL_IP := $(shell ifconfig | grep "inet " | grep -v 127.0.0.1 | awk '{print $$2}' | head -n 1)

# Set FOAMER_API_URL based on ENV
ifeq ($(ENV),local)
    export FOAMER_API_URL := http://$(LOCAL_IP):8080
else
    # Use environment variable if set, otherwise empty
    export FOAMER_API_URL ?=
endif

.PHONY: compile upload monitor clean compiledb show-api-url

show-api-url:
	@echo "FOAMER_API_URL=$(FOAMER_API_URL)"

compile:
	@echo "Building with FOAMER_API_URL=$(FOAMER_API_URL)"
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
