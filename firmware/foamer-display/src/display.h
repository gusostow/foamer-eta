#ifndef DISPLAY_H
#define DISPLAY_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

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

// Helper function to initialize mxcfg with clkphase before display construction
inline HUB75_I2S_CFG initConfig() {
  HUB75_I2S_CFG cfg(96, 48, 1, PINMAP);
  cfg.clkphase = false; // sample on falling edge to fix ghosting
  return cfg;
}

// Function to create and initialize the display object
MatrixPanel_I2S_DMA* createDisplay();

#endif
