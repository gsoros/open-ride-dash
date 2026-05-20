#pragma once

// see TFT_eSPI/User_Setup.h for reference

#define USER_SETUP_INFO ""  // Not sure if this is used anywhere

/*
  TFT panel configuration
  */
#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 240

/*
  Pin definitions for the display, these are already defined in pins.h but we cannot include that here because TFT_eSPI library uses preprocessor macros for pin definitions.
  */
#define TFT_MISO -1
#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_CS -1
#define TFT_DC 4
#define TFT_RST 1
// #define TFT_BL -1 // We will control backlight brightness via software PWM on this pin, so we will set it up in the driver constructor instead of using the TFT_eSPI library's built-in backlight control.

/*
  Font definitions
  */
// Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
// #define LOAD_GLCD

// Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
// #define LOAD_FONT4

// Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
// #define LOAD_FONT8

// FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
#define LOAD_GFXFF

/*
  Misc options
  */
#define SPI_FREQUENCY 27000000
