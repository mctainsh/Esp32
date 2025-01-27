#pragma once

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

#define MAX_LOG_LENGTH (200)
#define MAX_LOG_SIZE (MAX_LOG_LENGTH * 80)

#define RTK_SERVERS 3

#define APP_VERSION "1.89"