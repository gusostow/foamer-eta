#ifndef NETWORK_H
#define NETWORK_H

#include "net_timeouts.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Initialize WiFi connection
bool setupWiFi(const char *ssid, const char *password) {
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  for (int i = 0; i < 10; i++) {

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("Connected to WiFi");
      return true;
    }
    Serial.print(".");
    if (i < 9)
      delay(500);
  }
  Serial.println("WiFi connection failed");
  return false;
}

// Create and configure HTTP client for HTTPS URLs
WiFiClientSecure *createSecureClient() {
  WiFiClientSecure *client = new WiFiClientSecure();
  client->setInsecure(); // Skip certificate verification for simplicity
  // bound connect/handshake so a dropping firewall can't hang the fetch
  client->setTimeout(NET_TCP_TIMEOUT_S);
  client->setHandshakeTimeout(NET_TLS_HANDSHAKE_TIMEOUT_S);
  return client;
}

// Create HTTP client for HTTP URLs
WiFiClient *createClient() { return new WiFiClient(); }

#endif // NETWORK_H
