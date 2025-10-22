#include <Adafruit_GFX.h> // dependency
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ArduinoJson.h>

/* Test JSON data (embedded from foamer-example.json) */
const char STATIC_JSON[] PROGMEM = R"({
  "routes": [
    {
      "name": "Red",
      "mode": "METRORail",
      "color": "e41937",
      "directions": [
        {
          "headsign": "Southbound",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 10
            },
            {
              "type": "Scheduled",
              "minutes": 22
            },
            {
              "type": "Scheduled",
              "minutes": 34
            }
          ]
        },
        {
          "headsign": "Northbound",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 2
            },
            {
              "type": "Scheduled",
              "minutes": 15
            },
            {
              "type": "Scheduled",
              "minutes": 33
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
          "headsign": "Eastbound",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 8
            },
            {
              "type": "Scheduled",
              "minutes": 488
            },
            {
              "type": "Scheduled",
              "minutes": 539
            }
          ]
        },
        {
          "headsign": "Westbound",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 28
            },
            {
              "type": "Scheduled",
              "minutes": 76
            },
            {
              "type": "Scheduled",
              "minutes": 516
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
          "headsign": "Northbound",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 19
            },
            {
              "type": "Scheduled",
              "minutes": 45
            },
            {
              "type": "Scheduled",
              "minutes": 75
            }
          ]
        },
        {
          "headsign": "Southbound",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 26
            },
            {
              "type": "RealTime",
              "minutes": 44
            },
            {
              "type": "Scheduled",
              "minutes": 75
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
          "headsign": "Northbound",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 7
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
          "headsign": "Southbound",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 11
            },
            {
              "type": "RealTime",
              "minutes": 46
            },
            {
              "type": "Scheduled",
              "minutes": 71
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
          "headsign": "Eastbound",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 3
            },
            {
              "type": "RealTime",
              "minutes": 20
            },
            {
              "type": "RealTime",
              "minutes": 52
            }
          ]
        },
        {
          "headsign": "Westbound",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 2
            },
            {
              "type": "Scheduled",
              "minutes": 31
            },
            {
              "type": "Scheduled",
              "minutes": 61
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
          "headsign": "Eastbound",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 23
            },
            {
              "type": "RealTime",
              "minutes": 56
            },
            {
              "type": "Scheduled",
              "minutes": 82
            }
          ]
        },
        {
          "headsign": "Westbound",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 17
            },
            {
              "type": "Scheduled",
              "minutes": 47
            },
            {
              "type": "Scheduled",
              "minutes": 77
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
          "headsign": "Eastbound",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 7
            },
            {
              "type": "RealTime",
              "minutes": 39
            },
            {
              "type": "RealTime",
              "minutes": 61
            }
          ]
        },
        {
          "headsign": "Westbound",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 22
            },
            {
              "type": "Scheduled",
              "minutes": 52
            },
            {
              "type": "Scheduled",
              "minutes": 82
            }
          ]
        }
      ]
    }
  ]
})";

/* MatrixPortal-S3 ↔ HUB75 pin map */
const HUB75_I2S_CFG::i2s_pins PINMAP = {
    42, 41, 40, // R1 G1 B1
    38, 39, 37, // R2 G2 B2
    45, 36, 48, // A  B  C
    35, 21,     // D  E
    47, 14, 2   // LAT OE CLK
};

/* Panel configuration --------------------------------------------------- */
HUB75_I2S_CFG mxcfg(96, 48, 1, PINMAP); // width, height, chains, pins

void setup(void) {
  Serial.begin(115200);
  delay(2000); // Give serial time to connect

  Serial.println("Parsing JSON...");

  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, STATIC_JSON);

  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Get first route
  JsonObject route = doc["routes"][0];
  const char* routeName = route["name"];
  const char* routeMode = route["mode"];

  Serial.print("First route: ");
  Serial.print(routeName);
  Serial.print(" ");
  Serial.println(routeMode);

  // This fixes ghosting
  mxcfg.clkphase = false; // sample on falling edge

  MatrixPanel_I2S_DMA display(mxcfg);
  if (!display.begin()) {
    Serial.println("DMA init failed");
    for (;;)
      ;
  }

  display.setBrightness8(80); // keep < panel-width (96) to avoid ghosting

  /* ---- draw route header ---- */
  display.fillScreen(0);
  display.setTextWrap(false);

  // Route header at top (compact)
  display.setCursor(0, 0);
  display.setTextColor(display.color565(255, 255, 255)); // white for now
  display.setTextSize(1);

  // Display route name only (no mode to save space)
  String header = String(routeName);
  header.toUpperCase();

  display.print(header);

  Serial.print("Display header: ");
  Serial.println(header);

  // Get first direction
  JsonObject direction = route["directions"][0];
  const char* headsign = direction["headsign"];
  JsonArray departures = direction["departures"];

  // Draw first direction (Y position ~8 pixels from top)
  display.setCursor(0, 8);
  display.setTextColor(display.color565(100, 200, 255)); // cyan for direction

  // Abbreviate direction headsign
  String dir = String(headsign);
  dir.toUpperCase();

  // Shorten common direction names
  if (dir == "SOUTHBOUND") dir = "S";
  else if (dir == "NORTHBOUND") dir = "N";
  else if (dir == "EASTBOUND") dir = "E";
  else if (dir == "WESTBOUND") dir = "W";

  display.print(dir);
  display.print("|");

  // Departure times (up to 3, no comma spaces)
  int depCount = 0;
  for (JsonObject dep : departures) {
    if (depCount >= 3) break;

    int minutes = dep["minutes"];
    if (depCount > 0) {
      display.print(",");
    }
    display.print(minutes);
    depCount++;
  }

  Serial.print("First direction: ");
  Serial.print(headsign);
  Serial.print(" | ");
  Serial.print(depCount);
  Serial.println(" departures");
}

void loop() { /* nothing */
}
