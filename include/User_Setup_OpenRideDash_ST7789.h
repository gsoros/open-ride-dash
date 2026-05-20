#pragma once

#define USER_SETUP_INFO "Open Ride Dash ST7789"

#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 240

#define TFT_MISO -1
#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_CS -1
#define TFT_DC 4
#define TFT_RST 1
// #define TFT_BL -1

#define LOAD_GLCD

#define SPI_FREQUENCY 27000000

/*
#pragma once

#include "pins.h"

#define USER_SETUP_INFO "Open Ride Dash ST7789"

#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 240

#define TFT_MISO -1
#define TFT_SCLK SPI_SCK

#define LOAD_GLCD

#define SPI_FREQUENCY 27000000
*/