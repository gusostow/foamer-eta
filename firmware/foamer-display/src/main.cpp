// Include directives - <> means search in library/system paths
#include <Adafruit_GFX.h> // Adafruit graphics library (class-based)
#include <ArduinoJson.h>  // JSON parsing library
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h> // LED matrix driver

/* Test JSON data (embedded from foamer-example.json) */
// PROGMEM = store in flash memory instead of RAM (ESP32-specific)
// R"(...)" = raw string literal (C++11), no escape sequences needed
const char STATIC_JSON[] PROGMEM = R"({
  "routes": [
    {
      "name": "Red",
      "mode": "METRORail",
      "color": "e41937",
      "directions": [
        {
          "headsign": "Fannin South",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 0
            },
            {
              "type": "Scheduled",
              "minutes": 12
            },
            {
              "type": "Scheduled",
              "minutes": 24
            }
          ]
        },
        {
          "headsign": "North Line TC",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 4
            },
            {
              "type": "Scheduled",
              "minutes": 16
            },
            {
              "type": "Scheduled",
              "minutes": 28
            }
          ]
        }
      ]
    },
    {
      "name": "5",
      "mode": "Bus",
      "color": "2da646",
      "directions": [
        {
          "headsign": "Richey St",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 6
            },
            {
              "type": "Scheduled",
              "minutes": 43
            },
            {
              "type": "Scheduled",
              "minutes": 88
            }
          ]
        },
        {
          "headsign": "Wheeler TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 26
            },
            {
              "type": "RealTime",
              "minutes": 71
            }
          ]
        }
      ]
    },
    {
      "name": "11",
      "mode": "Bus",
      "color": "045bae",
      "directions": [
        {
          "headsign": "Gellhorn / 610",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 15
            },
            {
              "type": "Scheduled",
              "minutes": 50
            },
            {
              "type": "Scheduled",
              "minutes": 80
            }
          ]
        },
        {
          "headsign": "Hiram Clarke TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 5
            },
            {
              "type": "RealTime",
              "minutes": 27
            },
            {
              "type": "Scheduled",
              "minutes": 56
            }
          ]
        }
      ]
    },
    {
      "name": "56",
      "mode": "Bus",
      "color": "d94b3d",
      "directions": [
        {
          "headsign": "Greenspoint TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 8
            },
            {
              "type": "Scheduled",
              "minutes": 38
            },
            {
              "type": "Scheduled",
              "minutes": 68
            }
          ]
        },
        {
          "headsign": "TMC TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 21
            },
            {
              "type": "RealTime",
              "minutes": 51
            },
            {
              "type": "RealTime",
              "minutes": 64
            }
          ]
        }
      ]
    },
    {
      "name": "4",
      "mode": "Bus",
      "color": "d94b3d",
      "directions": [
        {
          "headsign": "Eastwood TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 11
            },
            {
              "type": "RealTime",
              "minutes": 37
            },
            {
              "type": "Scheduled",
              "minutes": 42
            }
          ]
        },
        {
          "headsign": "Mission Bend TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 12
            },
            {
              "type": "RealTime",
              "minutes": 17
            },
            {
              "type": "Scheduled",
              "minutes": 32
            }
          ]
        }
      ]
    },
    {
      "name": "65",
      "mode": "Bus",
      "color": "005db4",
      "directions": [
        {
          "headsign": "Wheeler TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 0
            },
            {
              "type": "RealTime",
              "minutes": 30
            },
            {
              "type": "RealTime",
              "minutes": 33
            }
          ]
        },
        {
          "headsign": "Synott Rd",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 2
            },
            {
              "type": "Scheduled",
              "minutes": 17
            },
            {
              "type": "Scheduled",
              "minutes": 32
            }
          ]
        }
      ]
    },
    {
      "name": "USS",
      "mode": "Bus",
      "color": "76fa8c",
      "directions": [
        {
          "headsign": "Target and Fiesta Shopping Center",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 16
            },
            {
              "type": "Scheduled",
              "minutes": 47
            }
          ]
        }
      ]
    },
    {
      "name": "25",
      "mode": "Bus",
      "color": "005db4",
      "directions": [
        {
          "headsign": "Eastwood TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 1
            },
            {
              "type": "RealTime",
              "minutes": 15
            },
            {
              "type": "RealTime",
              "minutes": 34
            }
          ]
        },
        {
          "headsign": "Westchase",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 3
            },
            {
              "type": "Scheduled",
              "minutes": 18
            },
            {
              "type": "Scheduled",
              "minutes": 33
            }
          ]
        }
      ]
    }
  ]
})";

/* Display configuration constants */
const int HEADSIGN_WIDTH = 7;
const char* REALTIME_COLOR = "3ac364";
const int DISPLAY_INTERVAL_MS = 3000; // 10 seconds per page

// Global variables for rotation
int currentRouteIndex = 0;
int totalRoutes = 0;

/* MatrixPortal-S3 ↔ HUB75 pin map */
// :: is the scope resolution operator - accesses nested type 'i2s_pins' inside class 'HUB75_I2S_CFG'
// This is like accessing a struct member in C, but for types defined inside classes
const HUB75_I2S_CFG::i2s_pins PINMAP = {
    42, 41, 40, // R1 G1 B1
    38, 39, 37, // R2 G2 B2
    45, 36, 48, // A  B  C
    35, 21,     // D  E
    47, 14, 2   // LAT OE CLK
};

/* Panel configuration --------------------------------------------------- */
// HUB75_I2S_CFG is a CLASS (like a struct with functions)
// This creates an OBJECT/INSTANCE of that class by calling its CONSTRUCTOR
// Constructor syntax: ClassName varName(arg1, arg2, ...)
// Similar to: HUB75_I2S_CFG mxcfg; mxcfg_init(&mxcfg, 96, 48, 1, PINMAP); in C
HUB75_I2S_CFG mxcfg(96, 48, 1, PINMAP); // width, height, chains, pins

// Helper function to initialize mxcfg with clkphase before display construction
HUB75_I2S_CFG initConfig() {
  HUB75_I2S_CFG cfg(96, 48, 1, PINMAP);
  cfg.clkphase = false; // sample on falling edge to fix ghosting
  return cfg;
}

// Create display object as global variable
MatrixPanel_I2S_DMA display(initConfig());

// Convert hex color string (e.g., "2da646") to RGB565 color
uint16_t hexToColor565(const char* hex) {
  // Parse hex string to RGB components
  long hexValue = strtol(hex, NULL, 16);
  uint8_t r = (hexValue >> 16) & 0xFF;
  uint8_t g = (hexValue >> 8) & 0xFF;
  uint8_t b = hexValue & 0xFF;
  return display.color565(r, g, b);
}

void displayDirection(MatrixPanel_I2S_DMA &display, JsonObject direction, const char* color) {
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
    display.setTextColor(hexToColor565(color));
    display.print(displayHeadsign);

    // Display separator in white
    display.setTextColor(display.color565(255, 255, 255));
    display.print("|");

    int depCount = 0;

    for (JsonObject dep : departures) {
        if(depCount == 3) break;

        const char* type = dep["type"];
        int minutes = dep["minutes"];

        if (depCount > 0) {
            // Comma always in white
            display.setTextColor(display.color565(255, 255, 255));
            display.print(",");
        }

        // Set color based on departure type
        if (strcmp(type, "RealTime") == 0) {
            display.setTextColor(hexToColor565(REALTIME_COLOR));
        } else {
            display.setTextColor(display.color565(255, 255, 255));
        }

        display.print(minutes);

        depCount++;
    }
    display.print("\n");
}

/* Function to display a route on the LED matrix */
void displayRoute(MatrixPanel_I2S_DMA &display, JsonObject route) {
    const char* name = route["name"];
    String routeName = String(name);
    const char* mode = route["mode"];
    String routeMode = String(mode);
    const char* color = route["color"];

    // Display route name and mode in route color
    display.setTextColor(hexToColor565(color));
    routeName.toUpperCase();
    display.print(routeName);
    display.print(" ");
    display.print(routeMode);
    display.print("\n");

    JsonArray directions = route["directions"];

    // Display first two directions (or less if not available)
    for (int i = 0; i < 2; i++) {
        if (i < directions.size()) {
            JsonObject direction = directions[i];
            displayDirection(display, direction, color);
        } else {
            // Write empty line if direction doesn't exist
            display.print("\n");
        }
    }

}


void setup(void) {
  // Serial is a global OBJECT (instance of a class)
  // .begin() is a METHOD (member function) of the Serial class
  // Dot notation: object.method() - calls a function that belongs to that object
  Serial.begin(115200);
  delay(2000); // Give serial time to connect

  Serial.println("Parsing JSON...");

  // Parse JSON once to get route count
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, STATIC_JSON);

  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }

  totalRoutes = doc["routes"].size();
  Serial.print("Total routes: ");
  Serial.println(totalRoutes);

  // Call .begin() METHOD on display object
  if (!display.begin()) {
    Serial.println("DMA init failed");
    for (;;)
      ;
  }

  display.setBrightness8(120);
  display.setTextWrap(false);
  display.setTextSize(1);
}

void loop() {
  // Parse JSON each iteration (local variable on stack)
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, STATIC_JSON);

  if (error) {
    Serial.println("JSON parse error in loop");
    delay(1000);
    return;
  }

  JsonArray routes = doc["routes"];

  // Clear screen and reset cursor
  display.fillScreen(0);
  display.setCursor(0, 0);
  display.setTextColor(display.color565(255, 255, 255));

  // Display two routes starting from currentRouteIndex
  for (int i = 0; i < 2 && (currentRouteIndex + i) < totalRoutes; i++) {
    JsonObject route = routes[currentRouteIndex + i];
    displayRoute(display, route);
  }

  // Wait for display interval
  delay(DISPLAY_INTERVAL_MS);

  // Move to next pair of routes
  currentRouteIndex += 2;

  // Loop back to start when we reach the end
  if (currentRouteIndex >= totalRoutes) {
    currentRouteIndex = 0;
  }
}
