#pragma once

// The TTGO T-Display has the following pins
#if USER_SETUP_ID == 25
#define BUTTON_1 35
#define BUTTON_2 0
#endif

// The TTGO T-Display-S3 has the following pins
#if USER_SETUP_ID == 206
#define BUTTON_1 14
#define BUTTON_2 0
#endif

#define MAX_LOG_LENGTH (100)

#define RTK_SERVERS 3

#define APP_VERSION "1.80.1"