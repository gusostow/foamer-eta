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
        },
        {
          "headsign": "North Line TC",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 8
            },
            {
              "type": "Scheduled",
              "minutes": 20
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
      "name": "5",
      "mode": "Bus",
      "color": "2da646",
      "directions": [
        {
          "headsign": "Richey St",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 32
            },
            {
              "type": "Scheduled",
              "minutes": 77
            },
            {
              "type": "Scheduled",
              "minutes": 122
            }
          ]
        },
        {
          "headsign": "Wheeler TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 14
            },
            {
              "type": "Scheduled",
              "minutes": 59
            },
            {
              "type": "Scheduled",
              "minutes": 104
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
              "minutes": 26
            },
            {
              "type": "Scheduled",
              "minutes": 54
            },
            {
              "type": "Scheduled",
              "minutes": 84
            }
          ]
        },
        {
          "headsign": "Hiram Clarke TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 0
            },
            {
              "type": "RealTime",
              "minutes": 32
            },
            {
              "type": "Scheduled",
              "minutes": 60
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
              "type": "Scheduled",
              "minutes": 12
            },
            {
              "type": "Scheduled",
              "minutes": 42
            },
            {
              "type": "Scheduled",
              "minutes": 72
            }
          ]
        },
        {
          "headsign": "TMC TC",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 14
            },
            {
              "type": "RealTime",
              "minutes": 50
            },
            {
              "type": "RealTime",
              "minutes": 74
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
              "minutes": 14
            },
            {
              "type": "RealTime",
              "minutes": 34
            },
            {
              "type": "RealTime",
              "minutes": 50
            }
          ]
        },
        {
          "headsign": "Mission Bend TC",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 6
            },
            {
              "type": "Scheduled",
              "minutes": 21
            },
            {
              "type": "Scheduled",
              "minutes": 36
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
              "minutes": 11
            },
            {
              "type": "RealTime",
              "minutes": 18
            },
            {
              "type": "RealTime",
              "minutes": 31
            }
          ]
        },
        {
          "headsign": "Synott Rd",
          "departures": [
            {
              "type": "Scheduled",
              "minutes": 6
            },
            {
              "type": "Scheduled",
              "minutes": 21
            },
            {
              "type": "Scheduled",
              "minutes": 36
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
              "minutes": 17
            },
            {
              "type": "Scheduled",
              "minutes": 48
            },
            {
              "type": "Scheduled",
              "minutes": 78
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
              "minutes": 9
            },
            {
              "type": "RealTime",
              "minutes": 24
            },
            {
              "type": "RealTime",
              "minutes": 33
            }
          ]
        },
        {
          "headsign": "Westchase",
          "departures": [
            {
              "type": "RealTime",
              "minutes": 5
            },
            {
              "type": "Scheduled",
              "minutes": 22
            },
            {
              "type": "Scheduled",
              "minutes": 37
            }
          ]
        }
      ]
    }
  ]
})";

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

void setup(void) {
  // Serial is a global OBJECT (instance of a class)
  // .begin() is a METHOD (member function) of the Serial class
  // Dot notation: object.method() - calls a function that belongs to that object
  Serial.begin(115200);
  delay(2000); // Give serial time to connect

  Serial.println("Parsing JSON...");

  // Parse JSON
  // JsonDocument is a CLASS from ArduinoJson library
  // 'doc' is an OBJECT (instance) created with default constructor (no arguments)
  JsonDocument doc;

  // deserializeJson is a regular function (not a method)
  // It returns a DeserializationError object
  DeserializationError error = deserializeJson(doc, STATIC_JSON);

  if (error) {
    Serial.print("JSON parse failed: ");
    // .c_str() is a METHOD that converts the error object to a C-style string (char*)
    Serial.println(error.c_str());
    return;
  }

  // Get first route
  // doc["routes"] uses operator[] overloading (C++ feature)
  // In C you'd do: get_element(doc, "routes")
  // JsonObject is a CLASS representing a JSON object
  JsonObject route = doc["routes"][0];

  // More operator[] usage - gets value and implicitly converts to const char*
  const char *routeName = route["name"];
  const char *routeMode = route["mode"];

  Serial.print("First route: ");
  Serial.print(routeName);
  Serial.print(" ");
  Serial.println(routeMode);

  // This fixes ghosting
  // .clkphase is a MEMBER VARIABLE (field) of the mxcfg object
  // Access with dot notation: object.member = value
  mxcfg.clkphase = false; // sample on falling edge

  // Create display object with mxcfg as constructor argument
  // MatrixPanel_I2S_DMA is a CLASS
  MatrixPanel_I2S_DMA display(mxcfg);

  // Call .begin() METHOD on display object
  // ! is logical NOT operator (same as C)
  if (!display.begin()) {
    Serial.println("DMA init failed");
    // Infinite loop (same as C)
    for (;;)
      ;
  }

  // Call METHOD .setBrightness8() with argument 80
  display.setBrightness8(80); // keep < panel-width (96) to avoid ghosting

  /* ---- draw route header ---- */
  // Calling METHODS on the display object
  display.fillScreen(0);
  display.setTextWrap(false);

  // Route header at top (compact)
  display.setCursor(0, 0);

  // Nested method calls: display.color565(...) returns a color value
  // That return value is passed to setTextColor()
  // This is METHOD CHAINING pattern
  display.setTextColor(display.color565(255, 255, 255)); // white for now
  display.setTextSize(1);

  // Display route name only (no mode to save space)
  // String is a CLASS (Arduino's string class, not std::string)
  // String(routeName) calls the CONSTRUCTOR that takes a const char*
  // Creates a String OBJECT from a C-style string
  String header = String(routeName);

  // .toUpperCase() is a METHOD that modifies the String object in-place
  // In C you'd write: str_to_upper(header);
  header.toUpperCase();

  // .print() METHOD - overloaded to accept many types (String, int, char*, etc)
  // FUNCTION OVERLOADING = same function name, different parameter types
  display.print(header);

  Serial.print("Display header: ");
  Serial.println(header);

  // Get first direction
  // JsonObject, JsonArray are CLASSes from ArduinoJson
  JsonObject direction = route["directions"][0];
  const char *headsign = direction["headsign"];
  JsonArray departures = direction["departures"];

  // Draw first direction (Y position ~8 pixels from top)
  display.setCursor(0, 8);
  display.setTextColor(display.color565(100, 200, 255)); // cyan for direction

  // Abbreviate direction headsign
  String dir = String(headsign);
  dir.toUpperCase();

  // Shorten common direction names
  // == operator is OVERLOADED for String class
  // In C you'd use: strcmp(dir, "SOUTHBOUND") == 0
  if (dir == "SOUTHBOUND")
    dir = "S";
  else if (dir == "NORTHBOUND")
    dir = "N";
  else if (dir == "EASTBOUND")
    dir = "E";
  else if (dir == "WESTBOUND")
    dir = "W";

  display.print(dir);
  display.print("|");

  // Departure times (up to 3, no comma spaces)
  int depCount = 0;

  // RANGE-BASED FOR LOOP (C++11 feature)
  // Similar to Python's: for dep in departures:
  // In C you'd write: for (int i = 0; i < departures.length; i++) { dep = departures[i]; ...}
  // JsonObject dep is created fresh each iteration
  for (JsonObject dep : departures) {
    if (depCount >= 3)
      break;

    // Implicit type conversion from JSON value to int
    int minutes = dep["minutes"];
    if (depCount > 0) {
      display.print(",");
    }
    display.print(minutes);

    // ++ is increment operator (same as C)
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
