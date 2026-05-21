#define ONBOARD_LED 8

// Attention: These pins are used by the TFT_eSPI library
// They are defined in TFT_eSPI_conf.h
// We are including them here for reference
#define SPI_SCK 6
#define SPI_MOSI 7
#define TFT_DC 4
#define TFT_RST 1
#define TFT_CS -1

// Backlight pin in handled by the display driver instead of
// using the TFT_eSPI library's built-in backlight control
#define TFT_BL 0
