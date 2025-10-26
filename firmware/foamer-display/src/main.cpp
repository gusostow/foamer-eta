// Include directives - <> means search in library/system paths
#include <Adafruit_GFX.h> // Adafruit graphics library (class-based)
#include <ArduinoJson.h>  // JSON parsing library
#include <WiFi.h>
#include <HTTPClient.h>
#include "display.h"

const char* foamer_api_host = FOAMER_API_HOST;

// WiFi credentials from environment variables
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
WiFiClient client;

/* Display configuration constants */
const int HEADSIGN_WIDTH = 7;
const char* REALTIME_COLOR = "3ac364";
const int DISPLAY_INTERVAL_MS = 10000; // 10 seconds per page

// Global variables for rotation
int currentRouteIndex = 0;
int totalRoutes = 0;
JsonDocument globalDoc; // Global to store fetched data
MatrixPanel_I2S_DMA* display; // Pointer to display object

// Convert hex color string (e.g., "2da646") to RGB565 color
uint16_t hexToColor565(const char* hex) {
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

  // TODO: Get lat/lon from config or make dynamic
  String url = String("http://") + foamer_api_host + "/departures?lat=40.68722&lon=-73.97481";

  Serial.print("Fetching: ");
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DeserializationError error = deserializeJson(doc, payload);
    http.end();

    if (error) {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return false;
    }
    Serial.println("Successfully fetched departures");
    return true;
  } else {
    Serial.print("HTTP request failed, code: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

void displayDirection(MatrixPanel_I2S_DMA* display, JsonObject direction, const char* color) {
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

    // Display headsign in route color
    display->setTextColor(hexToColor565(color));
    display->print(displayHeadsign);

    // Display separator in white
    display->setTextColor(display->color565(255, 255, 255));
    display->print("|");

    int depCount = 0;

    for (JsonObject dep : departures) {
        if(depCount == 3) break;

        const char* type = dep["type"];
        int minutes = dep["minutes"];

        if (depCount > 0) {
            // Comma always in white
            display->setTextColor(display->color565(255, 255, 255));
            display->print(",");
        }

        // Set color based on departure type
        if (strcmp(type, "RealTime") == 0) {
            display->setTextColor(hexToColor565(REALTIME_COLOR));
        } else {
            display->setTextColor(display->color565(255, 255, 255));
        }

        display->print(minutes);

        depCount++;
    }
    display->print("\n");
}

/* Function to display a route on the LED matrix */
void displayRoute(MatrixPanel_I2S_DMA* display, JsonObject route) {
    const char* name = route["name"];
    String routeName = String(name);
    const char* mode = route["mode"];
    String routeMode = String(mode);
    const char* color = route["color"];

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


void setup(void) {
  // Serial is a global OBJECT (instance of a class)
  // .begin() is a METHOD (member function) of the Serial class
  // Dot notation: object.method() - calls a function that belongs to that object
  Serial.begin(115200);
  delay(2000); // Give serial time to connect
               //
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to WiFi");

  // Create display object
  display = createDisplay();

  // Call .begin() METHOD on display object
  if (!display->begin()) {
    Serial.println("DMA init failed");
    for (;;)
      ;
  }

  display->setBrightness8(120);
  display->setTextWrap(false);
  display->setTextSize(1);
}

void loop() {
  // Fetch departures from API only at start of cycle
  if (currentRouteIndex == 0) {
    Serial.println("Fetching fresh data from API...");
    if (!fetchDepartures(globalDoc)) {
      Serial.println("Failed to fetch departures, retrying in 10s...");
      delay(10000);
      return;
    }

    JsonArray routes = globalDoc["routes"];
    totalRoutes = routes.size();
    Serial.print("Total routes: ");
    Serial.println(totalRoutes);
  }

  JsonArray routes = globalDoc["routes"];

  Serial.print("Current route index: ");
  Serial.println(currentRouteIndex);

  // Clear screen and reset cursor
  display->fillScreen(0);
  display->setCursor(0, 0);
  display->setTextColor(display->color565(255, 255, 255));

  // Display two routes starting from currentRouteIndex
  for (int i = 0; i < 2 && (currentRouteIndex + i) < totalRoutes; i++) {
    Serial.print("displaying route index ");
    Serial.println(currentRouteIndex + i);
    JsonObject route = routes[currentRouteIndex + i];
    displayRoute(display, route);
  }

  // Wait for display interval
  delay(DISPLAY_INTERVAL_MS);

  // Move to next pair of routes
  currentRouteIndex += 2;

  // Loop back to start when we reach the end
  if (currentRouteIndex >= totalRoutes) {
    Serial.println("Completed full cycle, will fetch fresh data on next iteration");
    currentRouteIndex = 0;
  }
}
