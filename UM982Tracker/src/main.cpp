#include <Arduino.h>
///////////////////////////////////////////////////////////////////////////////
//	  #########################################################################
//	  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
//	  #########################################################################
//
//	To use the TTGO T-Display with the TFT_eSPI library, you need to make the following changes to
//	the User_Setup.h file in the library.
//		.pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h
//
//	1. Comment out the default setup file
//		//#include <User_Setup.h>           // Default setup is root library folder
//	2. Uncomment the TTGO T-Display setup file
//		#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
//	3. Add the following line to the start of the file
//		#define DISABLE_ALL_LIBRARY_WARNINGS
///////////////////////////////////////////////////////////////////////////////

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>

#include <WiFi.h>
#include <iostream>
#include <sstream>
#include <string>

#include "Global.h"

#include "HandyString.h"
#include "MyDisplay.h"
#include "GpsParser.h"
#include "CredentialPrivate.h"
#include "NTRIPClient.h"
#include "MyFiles.h"
#include "ButtonInterrupt.h"

MyFiles _myFiles;
MyDisplay _display;
GpsParser _gpsParser(_display);
NTRIPClient _ntripClient(_display);

ButtonInterrupt _button1(BUTTON_1);/*, []()
							  { _button1.OnFallingIsr(); },
							   []()
							  { _button1.OnRaisingIsr(); });*/
ButtonInterrupt _button2(BUTTON_2);/*, []()
							  { _button2.OnFallingIsr(); },
							   []()
							  { _button2.OnRaisingIsr(); });*/

unsigned long _loopWaitTime = 0; // Time of last second
int _loopPersSecondCount = 0;	 // Number of times the main loops runs in a second

uint8_t _button1Current = HIGH; // Top button on left
uint8_t _button2Current = HIGH; // Bottom button when

// WiFi monitoring states
#define WIFI_STARTUP_TIMEOUT 20000
unsigned long _wifiFullResetTime = -WIFI_STARTUP_TIMEOUT;
wl_status_t _lastWifiStatus = wl_status_t::WL_NO_SHIELD;

bool IsButtonReleased(uint8_t button, uint8_t *pCurrent);
bool IsWifiConnected();

///////////////////////////////////////////////////////////////////////////////
// Setup
void setup(void)
{
	Serial.begin(115200);
	Serial.println("Starting");

	// Setup temporary startup display
	auto tft = TFT_eSPI();
	tft.init();
	tft.setRotation(1);
	tft.fillScreen(TFT_GREEN);
	tft.setTextColor(TFT_BLACK, TFT_GREEN);
	tft.setTextFont(2);
	tft.printf("Starting %\r\n", APP_VERSION);

#if T_DISPLAY_S3 == true
	Serial2.begin(115200, SERIAL_8N1, 12, 13);
	// Turn on display power for the TTGO T-Display-S3 (Needed for battery operation or if powered from 5V pin)
	pinMode(DISPLAY_POWER_PIN, OUTPUT);
	digitalWrite(DISPLAY_POWER_PIN, HIGH);
#else
	Serial2.begin(115200, SERIAL_8N1, 25, 26);
#endif

	_button1.AttachInterrupts([]() IRAM_ATTR { _button1.OnFallingIsr(); },[]() IRAM_ATTR { _button1.OnRaisingIsr(); });
	_button2.AttachInterrupts([]() IRAM_ATTR { _button2.OnFallingIsr(); },[]() IRAM_ATTR { _button2.OnRaisingIsr(); });

	// Verify file IO
	if (_myFiles.Setup())
	{
		//_myFiles.WriteFile("/hello.txt", "Hello ");
		//_myFiles.AppendFile("/hello.txt", "World!\r\n");

		// std::string response;
		//_myFiles.ReadFile("/hello.txt", response);
		// Serial.println(response.c_str());
	}

	_display.Setup();
	Serial.println("Startup Complete");
}

///////////////////////////////////////////////////////////////////////////////
// Loop here
void loop()
{
	// Trigger something every second
	int t = millis();
	_loopPersSecondCount++;
	if ((t - _loopWaitTime) > 1000)
	{
		_loopWaitTime = t;
		_display.SetLoopsPerSecond(_loopPersSecondCount);
		_loopPersSecondCount = 0;
	}

	// Check for push buttons
	if (_button1.WasPressed())
	// if (IsButtonReleased(BUTTON_1, &_button1Current))
	{
		Serial.println("Button 1");
		_display.NextPage();
	}
	if (_button2.WasPressed())
	// if (IsButtonReleased(BUTTON_2, &_button2Current))
	{
		Serial.println("Button 2");
		_display.ActionButton();
	}

	// Check for new data GPS serial data
	_display.SetGpsConnected(_gpsParser.ReadDataFromSerial(Serial2));

	// WIFI related functions
	if (IsWifiConnected())
	{
		// Check for new RTK corrections if we have a location already
		if (_display.HasLocation())
			_ntripClient.Loop();
	}

	// Update animations
	_display.Animate();
}

///////////////////////////////////////////////////////////////////////////////
// Check if the button is released. This takes < 1ms
// Note : Button is HIGH when released
bool IsButtonReleased(uint8_t button, uint8_t *pCurrent)
{
	if (*pCurrent != digitalRead(button))
	{
		*pCurrent = digitalRead(button);
		return *pCurrent == HIGH;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Check Wifi and reconnect
bool IsWifiConnected()
{
	// Is the WIFI connected?
	wl_status_t status = WiFi.status();
	if (_lastWifiStatus != status)
	{
		_lastWifiStatus = status;
		Serial.printf("Wifi Status %d %s\r\n", status, WifiStatus(status));
		_display.SetWebStatus(status);
		if (status == WL_CONNECTED)
			_display.SetRtkStatus("GPS Pending");
	}

	if (status == WL_CONNECTED)
		return true;

	// Start the connection process
	// Serial.println("E310 - No WIFI");
	unsigned long t = millis();
	unsigned long tDelta = t - _wifiFullResetTime;
	if (tDelta < WIFI_STARTUP_TIMEOUT)
	{

		// if( status != WL_DISCONNECTED)
		//	Serial.println(stateTitle.c_str());
		//_display.SetWebStatus(stateTitle.c_str());

		// Create a string of dots to show progress. 1 dot per second
		std::string dotsStr(tDelta / 1000, '.');
		_display.SetRtkStatus(dotsStr.c_str());

		return false;
	}

	// Reset the WIFI
	_wifiFullResetTime = t;
	WiFi.mode(WIFI_STA);
	wl_status_t beginState = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	//_display.SetWebStatus(WifiStatus(beginState));
	Serial.printf("WiFi Connecting %d %s\r\n", beginState, WifiStatus(beginState));

	return false;

	// Wait 20 seconds to connect
	// int countDown = 40;
	// while ((beginState = WiFi.status()) != WL_CONNECTED)
	//{
	//	_display.SetWebStatus(StringPrintf("%d %s", countDown, WifiStatus(beginState)));
	//	delay(500);
	//	Serial.print(".");
	//	if (countDown-- < 1)
	//	{
	//		Serial.println("\r\nE311 - WIFI Connect timed out");
	//		_display.SetWebStatus("NO WIFI!!!");
	//		return false;
	//	}
	//}
	//_display.SetWebStatus(WiFi.localIP().toString().c_str());
	// return true;
}

//  Check the Correct TFT Display Type is Selected in the User_Setup.h file
#if USER_SETUP_ID != 206 && USER_SETUP_ID != 25
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#endif