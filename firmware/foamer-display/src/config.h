#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h>

// Config class for accessing embedded configuration
class Config {
public:
  // Initialize and parse the embedded config JSON
  static bool begin();

  // WiFi settings
  static const char *getWifiSSID();
  static const char *getWifiPassword();

  // API settings
  static const char *getApiUrl();
  static const char *getApiSecret();

  // Geo settings
  static const char *getGeoLat();
  static const char *getGeoLon();

  // Display settings
  static int getPageIntervalMs();
  static int getMessageIntervalMs();

  // AWS IoT settings
  static bool isAwsIotEnabled();
  static const char *getAwsIotEndpoint();
  static const char *getAwsIotThingName();
  static const char *getAwsIotLogTopic();
  static const char *getAwsIotCertPem();
  static const char *getAwsIotPrivateKey();
  static const char *getAwsIotRootCa();

private:
  static JsonDocument doc;
  static bool initialized;
};

#endif // CONFIG_H
