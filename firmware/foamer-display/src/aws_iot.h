#ifndef AWS_IOT_H
#define AWS_IOT_H

#include "config.h"
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// Initialize AWS IoT connection
// Returns true if enabled and initialized successfully
bool setupAwsIot();

// Check if connected and reconnect if needed
// Call this in loop() to maintain connection
bool maintainAwsIotConnection();

// MQTT callback for incoming messages (currently unused, for future use)
void mqttCallback(char *topic, byte *payload, unsigned int length);

// Global MQTT client (defined in implementation)
extern PubSubClient *mqttClient;

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

  // Create MQTT client (static so it persists)
  static PubSubClient client(wifiClient);
  mqttClient = &client;

  // Configure MQTT broker
  const char *endpoint = Config::getAwsIotEndpoint();
  mqttClient->setServer(endpoint, 8883);
  mqttClient->setCallback(mqttCallback);

  // Increase buffer size for AWS IoT (default 128 is too small)
  mqttClient->setBufferSize(512);

  Serial.print("AWS IoT endpoint: ");
  Serial.println(endpoint);

  // Try initial connection
  if (connectToAwsIot()) {
    return true;
  } else {
    Serial.println("Initial AWS IoT connection failed, will retry...");
    return false;
  }
}

bool maintainAwsIotConnection() {
  if (!Config::isAwsIotEnabled() || !mqttClient) {
    return false;
  }

  if (!mqttClient->connected()) {
    Serial.println("AWS IoT disconnected, reconnecting...");
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
