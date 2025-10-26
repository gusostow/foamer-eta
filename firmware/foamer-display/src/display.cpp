#include "display.h"

// Function to create and initialize the display object
MatrixPanel_I2S_DMA* createDisplay() {
  return new MatrixPanel_I2S_DMA(initConfig());
}
