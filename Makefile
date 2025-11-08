# PlatformIO Makefile for foamer-display
# ESP32-S3 MatrixPortal board

PROJECT_DIR = firmware/foamer-display
PIO = platformio

# Config profile to use (dev, prod, etc.)
# Pass PROFILE=xxx to make to select a profile
PROFILE ?= dev
export CONFIG_PROFILE = $(PROFILE)

.PHONY: compile upload monitor clean compiledb embed provision

embed:
	@echo "embedding config and splash for profile: $(PROFILE)"
	@SERIAL=$$(cd $(PROJECT_DIR) && uv run python scripts/get-device-serial.py 2>/dev/null || echo ""); \
	if [ -n "$$SERIAL" ]; then \
		echo "device detected: $$SERIAL"; \
		export DEVICE_SERIAL=$$SERIAL; \
		cd $(PROJECT_DIR) && DEVICE_SERIAL=$$SERIAL uv run python scripts/embed_config.py; \
	else \
		echo "no device detected, embedding profile only"; \
		cd $(PROJECT_DIR) && uv run python scripts/embed_config.py; \
	fi

compile: embed
	@echo "building with profile: $(PROFILE)"
	cd $(PROJECT_DIR) && $(PIO) run

# Provision device if needed (checks for cached config)
provision:
	@echo "checking device provisioning..."
	@SERIAL=$$(cd $(PROJECT_DIR) && uv run python scripts/get-device-serial.py); \
	if [ -z "$$SERIAL" ]; then \
		echo "ERROR: Failed to read device USB serial number"; \
		exit 1; \
	fi; \
	echo "device serial: $$SERIAL"; \
	DEVICE_DIR="firmware/devices/$$SERIAL"; \
	if [ ! -d "$$DEVICE_DIR" ]; then \
		echo "device not provisioned, creating AWS IoT resources..."; \
		cd $(PROJECT_DIR) && uv run python scripts/provision-device.py --serial "$$SERIAL" --profile "$(PROFILE)"; \
	else \
		echo "device already provisioned: $$DEVICE_DIR"; \
	fi; \
	export DEVICE_SERIAL=$$SERIAL

upload: provision embed
	@echo "building with profile: $(PROFILE)"
	@SERIAL=$$(cd $(PROJECT_DIR) && uv run python scripts/get-device-serial.py); \
	export DEVICE_SERIAL=$$SERIAL; \
	cd $(PROJECT_DIR) && $(PIO) run -t upload

monitor:
	cd $(PROJECT_DIR) && $(PIO) device monitor

upload-monitor: provision embed
	@echo "building with profile: $(PROFILE)"
	cd $(PROJECT_DIR) && $(PIO) run -t upload -t monitor

clean:
	cd $(PROJECT_DIR) && $(PIO) run -t clean

compiledb:
	cd $(PROJECT_DIR) && $(PIO) run -t compiledb
