# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

foamer-eta is a web service and embedded system software for an LED transit time sign. The project fetches real-time transit departure data using the Transit API and provides an HTTP REST API for consumption by display hardware.

## Architecture

The project is a Rust workspace with four main crates:

### `transit` crate
Low-level client library for the Transit API (https://external.transitapp.com/v3). This crate:
- Provides a `TransitClient` that wraps HTTP calls to the Transit API
- Handles API authentication via the `TRANSIT_KEY` environment variable
- Defines comprehensive response types matching the Transit API schema
- Exposes three main endpoints: `nearby_stops`, `stop_departures`, and `nearby_routes`
- Returns raw Transit API responses with minimal transformation

### `messages` crate
Library crate for managing display messages stored in DynamoDB. This crate:
- Provides a `Client` struct for CRUD operations on messages
- Uses AWS SDK for DynamoDB operations
- Handles authentication via AWS environment variables
- Table name configured via `FOAMER_MESSAGES_TABLE` environment variable

### `api` crate
Library crate providing high-level domain logic. This crate:
- Provides a `Client` struct that wraps `TransitClient` and `MessagesClient` to combine transit data with display messages
- Main transformation: `Client::departures()` calls `TransitClient::nearby_routes()` and converts Transit API routes/itineraries/schedule_items into a simplified `Departures` structure with routes, directions, departure times, and an optional message
- Converts Unix timestamps to "minutes until departure" values
- Distinguishes between real-time and scheduled departures
- Defines domain models: `Departures`, `Route`, `Direction`, and `Departure`
- Includes `fmt` module with `lines()` function for word-wrapping messages to fit LED display width (16 chars)
- Message field in `Departures` is `Option<Vec<String>>` - pre-wrapped lines ready for display
- Default max_distance is 500 meters (defined in api/src/lib.rs)

### `svc` crate
HTTP REST service binary. This crate:
- Exposes an Axum-based HTTP service with a `/departures` endpoint
- The service accepts `lat`, `lon`, and optional `max_distance` query parameters
- Uses the `api` crate's `Client` for business logic
- Handles HTTP routing, request/response serialization, and error responses
- Includes tracing and logging configuration

Key architectural pattern: The `transit` crate owns all Transit API types and HTTP logic, the `api` crate provides simplified domain models suitable for display on an LED sign, and the `svc` crate handles HTTP concerns.

## Development Commands

### Rust Backend

#### Building
```bash
# Build entire workspace
LIBRARY_PATH=/Users/aostow/.nix-profile/lib:$LIBRARY_PATH cargo build

# Build specific crate
LIBRARY_PATH=/Users/aostow/.nix-profile/lib:$LIBRARY_PATH cargo build -p svc
LIBRARY_PATH=/Users/aostow/.nix-profile/lib:$LIBRARY_PATH cargo build -p api
LIBRARY_PATH=/Users/aostow/.nix-profile/lib:$LIBRARY_PATH cargo build -p transit
```

Note: The `LIBRARY_PATH` prefix is required in this environment. When using the Bash tool with cargo commands, this is already pre-approved for automatic execution.

#### Running
```bash
# Run the HTTP service (listens on 0.0.0.0:3000)
LIBRARY_PATH=/Users/aostow/.nix-profile/lib:$LIBRARY_PATH cargo run -p svc
```

The service requires the `TRANSIT_KEY` environment variable to be set. This is loaded from `.envrc.local` (not in git).

#### Testing
```bash
# Run all tests
cargo test

# Run tests for specific crate
cargo test -p svc
cargo test -p api
cargo test -p transit

# Run specific test
cargo test -p api test_departures

# Run integration tests only
cargo test --test integration_test
```

Integration tests require `TRANSIT_KEY` to be set. Tests use Houston, TX coordinates (29.72134736791465, -95.38383198936232) as test data.

#### API Usage Example
```bash
# Query departures for a location
curl "http://localhost:3000/departures?lat=29.7213&lon=-95.3838&max_distance=500"
```

### Arduino Firmware (ESP32-S3)

The `firmware/foamer-display` directory contains Arduino/PlatformIO code for the Adafruit MatrixPortal ESP32-S3 board with a 96x48 HUB75 LED matrix display.

**Note**: The Makefile is located at the project root. All `make` commands must be run from the project root directory.

#### Build System
The firmware uses PlatformIO with a Python build script that:
- Embeds configuration from JSON profiles (`firmware/profiles/*.json`)
- Generates splash screen bitmap from PNG image (`firmware/foamer-display/static/transit-logo.png`)
- Outputs generated headers: `src/config_data.h` and `src/splash.h`

Python dependencies are managed by `uv` (see `firmware/foamer-display/pyproject.toml`):
- Pillow (for image processing)

#### Configuration Profiles
Configuration is stored in `firmware/profiles/*.json`:
- `dev.json` - Development profile (default)
- `prod.json` - Production profile
- `example.json` - Template

Profile structure:
```json
{
  "wifi": {"ssid": "...", "password": "..."},
  "api": {"url": "https://...", "secret": "..."},
  "geo": {"lat": "...", "lon": "..."},
  "display": {
    "page_ms": 12000,          // Time per page in milliseconds
    "message_interval_ms": 60000  // Minimum time between message displays
  }
}
```

#### Building
```bash
# Compile firmware with default (dev) profile
make compile

# Compile with specific profile
make compile PROFILE=prod

# Run embed step only (regenerate config and splash headers)
make embed
```

The build process:
1. Runs `uv run python scripts/embed_config.py` to generate headers
2. Compiles firmware with PlatformIO

#### Uploading
```bash
# Compile and upload with default profile
make upload

# Upload with specific profile
make upload PROFILE=prod
```

#### Firmware Features
- **Splash screen**: Displays Transit app logo for 3 seconds on startup
- **WiFi connection**: Shows status messages during connection
- **Departure display**: Rotates through transit routes, 2 routes per page
- **Message display**: Shows wrapped messages from DynamoDB at configurable intervals
  - Single page (≤6 lines): Display for configured duration
  - Two pages (7-12 lines): Split across two screens
- **Configuration**: All settings loaded from embedded JSON at runtime

#### Required Libraries (auto-installed by PlatformIO)
- Adafruit GFX Library
- ESP32 HUB75 LED MATRIX PANEL DMA Display
- ArduinoJson

## Important Implementation Details

### Backend
- **Environment setup**: The project uses direnv with `.envrc` which sources `.envrc.local` for local environment variables (particularly `TRANSIT_KEY`)
- **Time handling**: Departure times from the Transit API are Unix timestamps (seconds since epoch). The api crate converts these to minutes-until-departure, handling edge cases like negative values (past departures become 0)
- **Route naming**: Routes prefer `route_short_name` over `route_long_name` for LED display brevity
- **Error handling**: Both crates use `anyhow::Result` for error propagation. The HTTP service converts errors to 500 responses with error messages
- **Async runtime**: Uses Tokio with full features enabled
- **Message wrapping**: Messages are wrapped server-side by `api::fmt::lines()` to avoid C++ string handling issues on the ESP32. Maximum width is 16 characters (LED display specific)

### Firmware
- **Configuration**: All settings are embedded at build time from JSON profiles. The `Config` class provides static getters for all configuration values
- **Splash screen**: RGB565 bitmap generated from PNG at build time, scaled to 37px with padding to fit 96x48 display
- **Message timing**: Messages are displayed at configurable intervals (`message_interval_ms`). They are skipped on first boot and only shown if the interval has elapsed since the last display
- **Display rotation**: The main loop rotates through transit routes showing 2 routes per page, with configurable page duration (`page_ms`)
- **Text wrapping**: The firmware receives pre-wrapped message lines from the API and displays them directly without text wrapping
- **Build dependencies**: The firmware build requires `uv` and Python (Pillow) for processing configuration and images. These are not needed at runtime on the device
- The CDK stacks are written in Python not typescript