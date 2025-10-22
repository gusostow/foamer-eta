#include <Adafruit_GFX.h> // dependency
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

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

  // This fixes ghosting
  mxcfg.clkphase = false; // sample on falling edge

  MatrixPanel_I2S_DMA display(mxcfg);
  if (!display.begin()) {
    Serial.println("DMA init failed");
    for (;;)
      ;
  }

  display.setBrightness8(80); // keep < panel-width (96) to avoid ghosting

  /* ---- draw some text ---- */
  display.fillScreen(0);
  display.setCursor(0, 0);
  display.setTextColor(display.color565(0, 255, 255));
  display.setTextWrap(true);
  display.print(F("Lorem ipsum dolo\n"
                  "r sit amet, co\n"
                  "nsectetur adip\n"
                  "iscing elit. S\n"
                  "ed do eiusmod \n"
                  "tempor incidid"));
}

void loop() { /* nothing */
}
