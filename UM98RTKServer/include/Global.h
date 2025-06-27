#pragma once
#include "HandyTime.h"

#define APP_VERSION "2.283.63"

// Disables logging to serial
#define SERIAL_LOG

// Enables the LC29HDA code (Comment out for UM980 and UM982)
//#define IS_LC29HDA

// The TTGO T-Display has the following pins
#if USER_SETUP_ID == 25
	#define T_DISPLAY_S2
	#define BUTTON_1 35
	#define BUTTON_2 0
#endif

// The TTGO T-Display-S3 has the following pins
#if USER_SETUP_ID == 206
	#define T_DISPLAY_S3
	#define BUTTON_1 14
	#define BUTTON_2 0
	#define DISPLAY_POWER_PIN 15
#endif

#define MAX_LOG_LENGTH (512)
#define MAX_LOG_SIZE (MAX_LOG_LENGTH * 80)
#define MAX_LOG_ROW_LENGTH (128 +24)

#define RTK_SERVERS 3

#define GPS_BUFFER_SIZE (16*1024)

// One 1 buttons on right, 3 Buttons on left
#define TFT_ROTATION  3

// WiFi access point password
#define AP_PASSWORD "John123456"
#define BASE_LOCATION_FILENAME "/BaseLocn.txt"
#define MDNS_HOST_FILENAME "/MDNS_HOST_Name.txt"

extern HandyTime _handyTime;
extern std::string _mdnsHostName;

