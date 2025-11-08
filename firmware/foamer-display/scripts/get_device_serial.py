#!/usr/bin/env python3
"""
Get USB serial number from connected ESP32 device.
Returns normalized serial number (no spaces, consistent format).
Non-disruptive - doesn't reset or communicate with device.
"""

import sys
import serial.tools.list_ports


def get_device_serial():
    """Get USB serial number from connected ESP32 device"""
    # List all USB serial ports
    ports = serial.tools.list_ports.comports()

    # Look for ESP32 devices
    esp32_ports = []
    for port in ports:
        # Check by VID:PID or description
        is_esp32 = (
            # CP210x USB-to-serial
            "10C4:EA60" in str(port)
            or
            # CH340 USB-to-serial
            "1A86:7523" in str(port)
            or
            # Native ESP32 USB (including ESP32-S3)
            "303A:" in str(port)
            or (port.vid == 0x303A)
            or
            # Check description for known ESP32 boards
            "ESP32" in (port.description or "").upper()
            or "MatrixPortal" in (port.description or "")
        )
        if is_esp32:
            esp32_ports.append(port)

    if not esp32_ports:
        print("ERROR: No ESP32 device found", file=sys.stderr)
        print("Available ports:", file=sys.stderr)
        for port in ports:
            print(f"  {port.device}: {port.description}", file=sys.stderr)
        sys.exit(1)

    if len(esp32_ports) > 1:
        print("WARNING: Multiple ESP32 devices found, using first:", file=sys.stderr)
        for port in esp32_ports:
            print(
                f"  {port.device}: {port.serial_number or 'no serial'}", file=sys.stderr
            )

    port = esp32_ports[0]

    if not port.serial_number:
        print("ERROR: Device has no USB serial number", file=sys.stderr)
        print(f"Device: {port.device}", file=sys.stderr)
        print(f"Description: {port.description}", file=sys.stderr)
        sys.exit(1)

    # Normalize serial number (remove spaces, lowercase)
    serial_number = port.serial_number.replace(" ", "").lower()
    return serial_number


if __name__ == "__main__":
    serial = get_device_serial()
    print(serial)
