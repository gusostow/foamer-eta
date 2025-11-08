// Include directives - <> means search in library/system paths
#include "config.h"
#include "display.h"
#include "network.h"
#include "splash.h"
#include "aws_iot.h"
#include <Adafruit_GFX.h> // Adafruit graphics library (class-based)
#include <ArduinoJson.h>  // JSON parsing library
#include <time.h>         // For NTP time sync

/* Display configuration constants */
const char *ERROR_COLOR = "D70000";
const int HEADSIGN_WIDTH = 6;
const char *TRANSIT_COLOR = "3ac364";
const char *MESSAGE_COLOR = "FF7B9C"; // Coral pink between peach and hot pink

// Global variables for rotation
int currentRouteIndex = 0;
int totalRoutes = 0;
unsigned long lastMessageTimeMs = 0; // Track when last message was displayed
JsonDocument globalDoc;              // Global to store fetched data
MatrixPanel_I2S_DMA *display;        // Pointer to display object

// Convert hex color string (e.g., "2da646") to RGB565 color
uint16_t hexToColor565(const char *hex) {
  // Parse hex string to RGB components
  long hexValue = strtol(hex, NULL, 16);
  uint8_t r = (hexValue >> 16) & 0xFF;
  uint8_t g = (hexValue >> 8) & 0xFF;
  uint8_t b = hexValue & 0xFF;
  return display->color565(r, g, b);
}

// Fetch departures from API
bool fetchDepartures(JsonDocument &doc) {
  HTTPClient http;

  String url = String(Config::getApiUrl()) +
               "/departures?lat=" + String(Config::getGeoLat()) +
               "&lon=" + String(Config::getGeoLon());

  Serial.print("Fetching: ");
  Serial.println(url);

  // Log API request
  String logMsg = "API request: " + url;
  log(LOG_DEBUG, logMsg.c_str());

  // Create client and track pointer for cleanup
  WiFiClient *client = nullptr;
  if (url.startsWith("https://")) {
    client = createSecureClient();
    http.begin(*static_cast<WiFiClientSecure *>(client), url);
  } else {
    client = createClient();
    http.begin(*client, url);
  }

  // Add API key header
  http.addHeader("x-api-key", Config::getApiSecret());

  int httpCode = http.GET();

  bool success = false;
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());

      // Log JSON parse error
      String logMsg = "API JSON parse failed: " + String(error.c_str());
      log(LOG_ERROR, logMsg.c_str());
    } else {
      Serial.println("Successfully fetched departures");
      success = true;
    }
  } else {
    Serial.print("HTTP request failed, code: ");
    Serial.println(httpCode);

    // Log HTTP error with response body
    String responseBody = http.getString();
    String logMsg = "API request failed: HTTP " + String(httpCode);
    if (responseBody.length() > 0 && responseBody.length() < 200) {
      logMsg += " - " + responseBody;
    } else if (responseBody.length() > 0) {
      logMsg += " - " + responseBody.substring(0, 200) + "...";
    }
    log(LOG_ERROR, logMsg.c_str());
  }

  http.end();
  delete client; // Free the allocated client memory

  return success;
}

void displayDirection(MatrixPanel_I2S_DMA *display, JsonObject direction,
                      const char *color) {
  const char *headsign = direction["headsign"];
  JsonArray departures = direction["departures"];

  String displayHeadsign = String(headsign);
  displayHeadsign.toUpperCase();
  if (displayHeadsign.length() > HEADSIGN_WIDTH) {
    displayHeadsign = displayHeadsign.substring(0, HEADSIGN_WIDTH);
  } else {
    // Pad with spaces to HEADSIGN_WIDTH characters
    while (displayHeadsign.length() < HEADSIGN_WIDTH) {
      displayHeadsign += " ";
    }
  }

  // Display bullet prefix in white
  display->setTextColor(display->color565(255, 255, 255));
  display->print("|");

  // Display headsign in route color
  display->setTextColor(hexToColor565(color));
  display->print(displayHeadsign);

  // Display separator in white
  display->setTextColor(display->color565(255, 255, 255));
  display->print(" ");

  int depCount = 0;

  for (JsonObject dep : departures) {
    if (depCount == 3)
      break;

    const char *type = dep["type"];
    int minutes = dep["minutes"];

    if (depCount > 0) {
      // Comma always in white
      display->setTextColor(display->color565(255, 255, 255));
      display->print(",");
    }

    // Set color based on departure type
    if (strcmp(type, "RealTime") == 0) {
      display->setTextColor(hexToColor565(TRANSIT_COLOR));
    } else {
      display->setTextColor(display->color565(255, 255, 255));
    }

    display->print(minutes);

    depCount++;
  }
  display->print("\n");
}

/* Function to display a message on the LED matrix */
void displayMessage(MatrixPanel_I2S_DMA *display, JsonArray messageLines) {
  int totalLines = messageLines.size();
  int linesPerPage = 6;

  if (totalLines <= linesPerPage) {
    // Single page - display all lines for 10s
    display->fillScreen(0);
    display->setCursor(0, 0);
    display->setTextColor(hexToColor565(MESSAGE_COLOR));
    for (JsonVariant line : messageLines) {
      const char *lineText = line.as<const char *>();
      display->println(lineText);
    }
    delay(20000);
  } else {
    // Two pages - 5s each for 10s total
    // Page 1: lines 0-5
    display->fillScreen(0);
    display->setCursor(0, 0);
    display->setTextColor(hexToColor565(MESSAGE_COLOR));
    for (int i = 0; i < linesPerPage && i < totalLines; i++) {
      const char *lineText = messageLines[i].as<const char *>();
      display->println(lineText);
    }
    delay(15000);

    // Page 2: lines 6-11
    display->fillScreen(0);
    display->setCursor(0, 0);
    display->setTextColor(hexToColor565(MESSAGE_COLOR));
    for (int i = linesPerPage; i < totalLines; i++) {
      const char *lineText = messageLines[i].as<const char *>();
      display->println(lineText);
    }
    delay(5000);
  }
}

/* Function to display a route on the LED matrix */
void displayRoute(MatrixPanel_I2S_DMA *display, JsonObject route) {
  const char *name = route["name"];
  String routeName = String(name);
  const char *mode = route["mode"];
  String routeMode = String(mode);
  const char *color = route["color"];

  // Display route name and mode in route color
  display->setTextColor(hexToColor565(color));
  routeName.toUpperCase();
  display->print(routeName);
  display->print(" ");
  display->print(routeMode);
  display->print("\n");

  JsonArray directions = route["directions"];

  // Display first two directions (or less if not available)
  for (int i = 0; i < 2; i++) {
    if (i < directions.size()) {
      JsonObject direction = directions[i];
      displayDirection(display, direction, color);
    } else {
      // Write empty line if direction doesn't exist
      display->print("\n");
    }
  }
}

/* Function to display splash screen at startup */
void displaySplash(MatrixPanel_I2S_DMA *display) {
  display->fillScreen(0);
  for (int y = 0; y < SPLASH_HEIGHT; y++) {
    for (int x = 0; x < SPLASH_WIDTH; x++) {
      uint16_t color = SPLASH_BITMAP[y * SPLASH_WIDTH + x];
      display->drawPixel(x, y, color);
    }
  }
  delay(3000);
}

void setup(void) {
  // Serial is a global OBJECT (instance of a class)
  // .begin() is a METHOD (member function) of the Serial class
  // Dot notation: object.method() - calls a function that belongs to that
  // object
  Serial.begin(115200);
  delay(2000); // Give serial time to connect

  // Initialize configuration
  if (!Config::begin()) {
    Serial.println("Config init failed");
    for (;;)
      ;
  }

  // Create display object
  display = createDisplay();

  // Call .begin() METHOD on display object
  if (!display->begin()) {
    Serial.println("DMA init failed");
    for (;;)
      ;
  }

  display->setBrightness8(120);
  display->setTextSize(1);
  display->setTextWrap(true);

  // Display splash screen first
  displaySplash(display);

  // Connect to WiFi
  while (!setupWiFi(Config::getWifiSSID(), Config::getWifiPassword())) {
    display->fillScreen(0);
    display->setCursor(0, 0);
    display->setTextColor(hexToColor565(ERROR_COLOR));
    display->println("WiFi error: ");
    display->println("");
    display->setTextColor(display->color565(255, 255, 255));
    display->println(Config::getWifiSSID());
    delay(5000);
  }

  // Show WiFi connected message while doing NTP and IoT setup
  display->fillScreen(0);
  display->setCursor(0, 0);
  display->setTextColor(display->color565(255, 255, 255));
  display->println("WiFi connected:");
  display->println("");
  display->setTextColor(hexToColor565(TRANSIT_COLOR));
  display->println(Config::getWifiSSID());

  // Sync time with NTP (required for TLS certificate validation)
  // Set timezone to US Central with automatic DST handling
  configTime(-6 * 3600, 3600, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CST6CDT,M3.2.0,M11.1.0", 1);
  tzset();

  time_t now = 0;
  struct tm timeinfo;
  int retry = 0;
  while (now < 1000000000 && retry < 20) {
    time(&now);
    delay(500);
    retry++;
  }

  bool ntp_failed = false;
  if (now < 1000000000) {
    log(LOG_ERROR, "NTP sync failed");
    ntp_failed = true;
  } else {
    localtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.println(asctime(&timeinfo));
    log(LOG_INFO, "NTP sync successful");
  }

  // Initialize AWS IoT if enabled
  bool iot_failed = false;
  if (Config::isAwsIotEnabled()) {
    if (!setupAwsIot()) {
      log(LOG_ERROR, "AWS IoT connection failed");
      iot_failed = true;
    } else {
      log(LOG_INFO, "AWS IoT connected");
    }
  }

  // Show errors if any occurred
  if (ntp_failed || iot_failed) {
    display->fillScreen(0);
    display->setCursor(0, 0);
    display->setTextColor(hexToColor565(ERROR_COLOR));
    if (ntp_failed) {
      display->println("NTP sync failed!");
    }
    if (iot_failed) {
      display->println("AWS IoT failed");
    }
    delay(3000);
  }

  display->fillScreen(0);
  display->setCursor(0, 0);
  display->setTextWrap(false);
}

void loop() {
  // Maintain AWS IoT connection
  maintainAwsIotConnection();

  // Fetch departures from API only at start of cycle
  if (currentRouteIndex == 0) {
    log(LOG_INFO, "Fetching departures from API");
    if (!fetchDepartures(globalDoc)) {
      log(LOG_ERROR, "Failed to fetch departures, retrying in 10s");
      delay(10000);
      return;
    }

    JsonArray routes = globalDoc["routes"];
    totalRoutes = routes.size();
    Serial.print("Total routes: ");
    Serial.println(totalRoutes);
    log(LOG_INFO, "Departures fetched successfully");

    // Check if there's a message to display
    JsonArray message = globalDoc["message"];
    if (!message.isNull()) {
      unsigned long currentTime = millis();
      unsigned long elapsed = currentTime - lastMessageTimeMs;

      if (elapsed >= Config::getMessageIntervalMs()) {
        Serial.println("Message to display:");
        for (JsonVariant line : message) {
          Serial.println(line.as<const char *>());
        }
        displayMessage(display, message);
        lastMessageTimeMs = millis();
        log(LOG_INFO, "Message displayed");
      } else {
        Serial.print("Skipping message, elapsed: ");
        Serial.print(elapsed);
        Serial.print("ms, interval: ");
        Serial.print(Config::getMessageIntervalMs());
        Serial.println("ms");
      }
    }
  }

  // Clear screen and reset cursor
  display->fillScreen(0);
  display->setCursor(0, 0);
  display->setTextColor(display->color565(255, 255, 255));

  // Display two routes starting from currentRouteIndex
  JsonArray routes = globalDoc["routes"];
  for (int i = 0; i < 2 && (currentRouteIndex + i) < totalRoutes; i++) {
    JsonObject route = routes[currentRouteIndex + i];
    displayRoute(display, route);
  }

  // Move to next pair of routes
  currentRouteIndex += 2;

  // Loop back to start when we reach the end
  if (currentRouteIndex >= totalRoutes) {
    currentRouteIndex = 0;
  }

  // Wait for display interval while maintaining IoT connection
  unsigned long startTime = millis();
  while (millis() - startTime < Config::getPageIntervalMs()) {
    maintainAwsIotConnection();
    delay(100); // Small delay to avoid tight loop
  }
}
