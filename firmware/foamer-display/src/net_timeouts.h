#ifndef NET_TIMEOUTS_H
#define NET_TIMEOUTS_H

#include <Arduino.h>

// Network operation timeouts, kept short so a firewall that silently drops
// traffic can't hang the boot or display loop.
//
// The core defaults are what caused indefinite hangs on restrictive networks:
// WiFiClientSecure defaults to a 30s TCP connect and a 120s TLS handshake, so a
// dropped connection to AWS IoT (TCP 8883) could block for up to two minutes.

// TCP connect / socket read timeout for TLS clients (seconds)
static const uint32_t NET_TCP_TIMEOUT_S = 10;

// TLS handshake timeout (seconds)
static const uint32_t NET_TLS_HANDSHAKE_TIMEOUT_S = 10;

// MQTT CONNACK read timeout (seconds)
static const uint16_t NET_MQTT_SOCKET_TIMEOUT_S = 5;

// HTTPClient connect + stream read timeout (milliseconds)
static const uint16_t NET_HTTP_TIMEOUT_MS = 10000;

// Minimum gap between AWS IoT reconnect attempts, so a persistently unreachable
// broker doesn't block the display loop on every pass
static const unsigned long AWS_IOT_RECONNECT_COOLDOWN_MS = 30000;

#endif // NET_TIMEOUTS_H
