# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

foamer-eta is a web service and embedded system software for an LED transit time sign. The project fetches real-time transit departure data using the Transit API and provides an HTTP REST API for consumption by display hardware.

## Architecture

The project is a Rust workspace with two main crates:

### `transit` crate
Low-level client library for the Transit API (https://external.transitapp.com/v3). This crate:
- Provides a `TransitClient` that wraps HTTP calls to the Transit API
- Handles API authentication via the `TRANSIT_KEY` environment variable
- Defines comprehensive response types matching the Transit API schema
- Exposes three main endpoints: `nearby_stops`, `stop_departures`, and `nearby_routes`
- Returns raw Transit API responses with minimal transformation

### `api` crate
High-level application logic and HTTP REST service. This crate:
- Provides a `Client` struct that wraps `TransitClient` and transforms data into simplified domain models
- Main transformation: `Client::departures()` calls `TransitClient::nearby_routes()` and converts Transit API routes/itineraries/schedule_items into a simplified `Departures` structure with routes, directions, and departure times
- Converts Unix timestamps to "minutes until departure" values
- Distinguishes between real-time and scheduled departures
- Exposes an Axum-based HTTP service in the `svc` module with a `/departures` endpoint
- The service accepts `lat`, `lon`, and optional `max_distance` query parameters
- Default max_distance is 500 meters (defined in api/src/lib.rs:8)

Key architectural pattern: The `transit` crate owns all Transit API types and HTTP logic, while the `api` crate provides simplified domain models suitable for display on an LED sign.

## Development Commands

### Building
```bash
# Build entire workspace
LIBRARY_PATH=/Users/aostow/.nix-profile/lib:$LIBRARY_PATH cargo build

# Build specific crate
LIBRARY_PATH=/Users/aostow/.nix-profile/lib:$LIBRARY_PATH cargo build -p api
LIBRARY_PATH=/Users/aostow/.nix-profile/lib:$LIBRARY_PATH cargo build -p transit
```

Note: The `LIBRARY_PATH` prefix is required in this environment. When using the Bash tool with cargo commands, this is already pre-approved for automatic execution.

### Running
```bash
# Run the HTTP service (listens on 0.0.0.0:3000)
LIBRARY_PATH=/Users/aostow/.nix-profile/lib:$LIBRARY_PATH cargo run -p api
```

The service requires the `TRANSIT_KEY` environment variable to be set. This is loaded from `.envrc.local` (not in git).

### Testing
```bash
# Run all tests
cargo test

# Run tests for specific crate
cargo test -p api
cargo test -p transit

# Run specific test
cargo test -p api test_departures

# Run integration tests only
cargo test --test integration_test
```

Integration tests require `TRANSIT_KEY` to be set. Tests use Houston, TX coordinates (29.72134736791465, -95.38383198936232) as test data.

### API Usage Example
```bash
# Query departures for a location
curl "http://localhost:3000/departures?lat=29.7213&lon=-95.3838&max_distance=500"
```

## Important Implementation Details

- **Environment setup**: The project uses direnv with `.envrc` which sources `.envrc.local` for local environment variables (particularly `TRANSIT_KEY`)
- **Time handling**: Departure times from the Transit API are Unix timestamps (seconds since epoch). The api crate converts these to minutes-until-departure, handling edge cases like negative values (past departures become 0)
- **Route naming**: Routes prefer `route_short_name` over `route_long_name` for LED display brevity
- **Error handling**: Both crates use `anyhow::Result` for error propagation. The HTTP service converts errors to 500 responses with error messages
- **Async runtime**: Uses Tokio with full features enabled
