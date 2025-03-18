#ifndef GLOBAL_H
#define GLOBAL_H

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>

#define APP_VERSION "2.198"

// The TTGO T-Display has the following pins
#if USER_SETUP_ID == 25
	#define T_DISPLAY_S3 false
	#define BUTTON_1 35
	#define BUTTON_2 0
#endif

// The TTGO T-Display-S3 has the following pins
#if USER_SETUP_ID == 206
	#define T_DISPLAY_S3 true
	#define BUTTON_1 14
	#define BUTTON_2 0
	#define DISPLAY_POWER_PIN 15
#endif

#define SCR_W TFT_HEIGHT
#define SCR_H TFT_WIDTH

const char *AP_PASSWORD = "JohnTLegend";

#endif // GLOBAL_H