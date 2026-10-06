#ifndef AWS_IOT_H
#define AWS_IOT_H

#include "config.h"
#include "net_timeouts.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// Log levels
const char* LOG_DEBUG = "DEBUG";
const char* LOG_INFO = "INFO";
const char* LOG_WARN = "WARNING";
const char* LOG_ERROR = "ERROR";

// Global MQTT client pointer (defined at bottom of file)
extern PubSubClient *mqttClient;

// true once the initial AWS IoT connect succeeds. best effort: if the first
// attempt fails we give up for the session and never block the loop again
static bool awsIotActive = false;

// Initialize AWS IoT connection
// Returns true if enabled and initialized successfully
bool setupAwsIot();

// Check if connected and reconnect if needed
// Call this in loop() to maintain connection
bool maintainAwsIotConnection();

// MQTT callback for incoming messages (currently unused, for future use)
void mqttCallback(char *topic, byte *payload, unsigned int length);

// Log a message to both Serial and AWS IoT (if connected)
void log(const char* level, const char* message) {
  // Always output to Serial
  Serial.print("[");
  Serial.print(level);
  Serial.print("] ");
  Serial.println(message);

  // If AWS IoT is enabled and connected, publish to MQTT
  if (Config::isAwsIotEnabled() && mqttClient && mqttClient->connected()) {
    // Create JSON log message with thing name for easy filtering
    JsonDocument doc;
    doc["timestamp"] = time(nullptr);
    doc["thing_name"] = Config::getAwsIotThingName();
    doc["level"] = level;
    doc["message"] = message;

    // Serialize to string
    String jsonString;
    serializeJson(doc, jsonString);

    // Publish to log topic (non-blocking)
    const char* logTopic = Config::getAwsIotLogTopic();
    mqttClient->publish(logTopic, jsonString.c_str());
  }
}

// Connect to AWS IoT MQTT broker
// Returns true on success
bool connectToAwsIot() {
  if (!Config::isAwsIotEnabled()) {
    return false;
  }

  const char *thingName = Config::getAwsIotThingName();
  Serial.print("Connecting to AWS IoT as ");
  Serial.println(thingName);

  // MQTT client ID must match thing name for policy ${iot:Connection.Thing.ThingName}
  if (mqttClient->connect(thingName)) {
    Serial.println("Connected to AWS IoT!");
    return true;
  } else {
    Serial.print("AWS IoT connection failed, rc=");
    Serial.println(mqttClient->state());
    return false;
  }
}

bool setupAwsIot() {
  if (!Config::isAwsIotEnabled()) {
    Serial.println("AWS IoT disabled in config");
    return false;
  }

  Serial.println("Initializing AWS IoT...");

  // Create secure WiFi client
  static WiFiClientSecure wifiClient;

  // Set certificates
  wifiClient.setCACert(Config::getAwsIotRootCa());
  wifiClient.setCertificate(Config::getAwsIotCertPem());
  wifiClient.setPrivateKey(Config::getAwsIotPrivateKey());

  // bound TCP connect + TLS handshake (defaults are 30s / 120s), otherwise a
  // firewall that drops port 8883 hangs here for up to two minutes
  wifiClient.setTimeout(NET_TCP_TIMEOUT_S);
  wifiClient.setHandshakeTimeout(NET_TLS_HANDSHAKE_TIMEOUT_S);

  // Create MQTT client (static so it persists)
  static PubSubClient client(wifiClient);
  mqttClient = &client;

  // bound the MQTT CONNACK read too
  mqttClient->setSocketTimeout(NET_MQTT_SOCKET_TIMEOUT_S);

  // Configure MQTT broker
  const char *endpoint = Config::getAwsIotEndpoint();
  mqttClient->setServer(endpoint, 8883);
  mqttClient->setCallback(mqttCallback);

  // Increase buffer size for AWS IoT (default 128 is too small)
  mqttClient->setBufferSize(512);

  // Set keepalive to 60 seconds (default is 15)
  mqttClient->setKeepAlive(60);

  Serial.print("AWS IoT endpoint: ");
  Serial.println(endpoint);

  // Try initial connection. best effort: if it fails now, give up for the
  // session rather than blocking the display loop with retries later
  if (connectToAwsIot()) {
    awsIotActive = true;
    return true;
  } else {
    Serial.println("Initial AWS IoT connection failed, giving up for session");
    awsIotActive = false;
    return false;
  }
}

bool maintainAwsIotConnection() {
  // give up entirely if the initial connect never succeeded (best effort)
  if (!Config::isAwsIotEnabled() || !mqttClient || !awsIotActive) {
    return false;
  }

  if (!mqttClient->connected()) {
    // throttle reconnects so an unreachable broker doesn't block the display
    // loop on every pass (maintain is called every ~100ms)
    static unsigned long lastReconnectAttemptMs = 0;
    unsigned long nowMs = millis();
    if (lastReconnectAttemptMs != 0 &&
        (nowMs - lastReconnectAttemptMs) < AWS_IOT_RECONNECT_COOLDOWN_MS) {
      return false;
    }
    lastReconnectAttemptMs = nowMs;

    log(LOG_WARN, "AWS IoT disconnected, reconnecting");
    if (!connectToAwsIot()) {
      return false;
    }
  }

  mqttClient->loop();
  return true;
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Global MQTT client pointer
PubSubClient *mqttClient = nullptr;

#endif // AWS_IOT_H
