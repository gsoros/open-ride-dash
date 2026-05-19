#pragma once

#include "pins.h"

#define USER_SETUP_INFO "Open Ride Dash ST7789"

#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 240

#define TFT_MISO -1
#define TFT_MOSI SPI_MOSI
#define TFT_SCLK SPI_SCK

#define LOAD_GLCD

#define SPI_FREQUENCY 27000000
