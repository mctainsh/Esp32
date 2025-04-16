#include <Arduino.h>
///////////////////////////////////////////////////////////////////////////////
//	  #########################################################################
//	  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
//	  #########################################################################
//
//	To use the TTGO T-Display or T-Display-S3 with the TFT_eSPI library, you need to make the following changes to
//	the .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h file in the library.
//
//	1. Comment out the default setup file
//		//#include <User_Setup.h>           // Default setup is root library folder
//
//	2. Uncomment the TTGO T-Display setup file
//		#include <User_Setups/Setup25_TTGO_T_Display.h>    		// Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
//	  	OR
//		#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>     // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT
//
//	3. Add the following line to the start of the file
//		#define DISABLE_ALL_LIBRARY_WARNINGS
///////////////////////////////////////////////////////////////////////////////

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>

#include <WiFi.h>
#include <iostream>
#include <sstream>
#include <string>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

void SaveBaseLocation(std::string newBaseLocation);
void LoadBaseLocation();

#include "Global.h"
#include "HandyLog.h"
#include "HandyString.h"
#include "MyDisplay.h"
#include "GpsParser.h"
#include "NTRIPServer.h"
#include "MyFiles.h"
#include <WebPortal.h>
#include "WifiBusyTask.h"
#include "WiFiEvents.h"
#include "History.h"

WiFiManager _wifiManager;

unsigned long _loopWaitTime = 0;	// Time of last second
int _loopPersSecondCount = 0;		// Number of times the main loops runs in a second
unsigned long _lastButtonPress = 0; // Time of last button press to turn off display on T-Display-S3
History _history;					// Temperature history

WebPortal _webPortal;

uint8_t _button1Current = HIGH; // Top button on left
uint8_t _button2Current = HIGH; // Bottom button when

MyFiles _myFiles;
MyDisplay _display;
GpsParser _gpsParser(_display);
NTRIPServer _ntripServer0(0);
NTRIPServer _ntripServer1(1);
NTRIPServer _ntripServer2(2);
std::string _baseLocation = "";

// WiFi monitoring states
#define WIFI_STARTUP_TIMEOUT 20000
unsigned long _wifiFullResetTime = -WIFI_STARTUP_TIMEOUT;
wl_status_t _lastWifiStatus = wl_status_t::WL_NO_SHIELD;

bool IsButtonReleased(uint8_t button, uint8_t *pCurrent);
bool IsWifiConnected();
String MakeHostName();

///////////////////////////////////////////////////////////////////////////////
// Setup
void setup(void)
{
	perror("RTL Server - Starting");
	perror(APP_VERSION);

	// Setup temporary startup display
	auto tft = TFT_eSPI();
	tft.init();
	tft.setRotation(TFT_ROTATION); // One 1 buttons on right, 3 Buttons on left
	tft.fillScreen(TFT_GREEN);
	tft.setTextColor(TFT_BLACK, TFT_GREEN);
	tft.setTextFont(2);
	tft.printf("Starting %. Cores %d\r\n", APP_VERSION, configNUM_CORES);

	SetupLog(); // Call this before any logging

	// No logging before here
	Serial.begin(115200); // Using perror() instead
	Logf("Starting %s. Cores:%d", APP_VERSION, configNUM_CORES);

	// Setup the serial buffer for the GPS port
	Logf("GPS Buffer size %d", Serial2.setRxBufferSize(GPS_BUFFER_SIZE));

	tft.println("Enable Display pins");
#if T_DISPLAY_S3 == true
	// Turn on display power for the TTGO T-Display-S3 (Needed for battery operation or if powered from 5V pin)
	pinMode(DISPLAY_POWER_PIN, OUTPUT);
	digitalWrite(DISPLAY_POWER_PIN, HIGH);
#endif

	Logln("Enable WIFI");
	tft.println("Enable WIFI");
	SetupWiFiEvents();
	WiFi.mode(WIFI_AP_STA);

	Logln("Enable Buttons");
	tft.println("Enable Buttons");
	pinMode(BUTTON_1, INPUT_PULLUP);
	pinMode(BUTTON_2, INPUT_PULLUP);

	// Verify file IO (This can take up tpo 60s is SPIFFs not initialised)
	tft.println("Setup SPIFFS");
	tft.println("This can take up to 60 seconds ...");
	if (_myFiles.Setup())
		Logln("Test file IO");
	else
		Logln("E100 - File IO failed");
	LoadBaseLocation();

	// Load the NTRIP server settings
	tft.println("Setup NTRIP Connections");
	_ntripServer0.LoadSettings();
	_ntripServer1.LoadSettings();
	_ntripServer2.LoadSettings();
	_gpsParser.Setup(&_ntripServer0, &_ntripServer1, &_ntripServer2);

	_display.Setup();
	Logf("Display type %d", USER_SETUP_ID);

	// Reset Wifi Setup if needed (Do tis to clear out old wifi credentials)
	//_wifiManager.erase();

	// Setup host name to have RTK_ prefix
	WiFi.setHostname(MakeHostName().c_str());
	_display.RefreshScreen();

	// Block here till we have WiFi credentials (good or bad)
	Logf("Start listening on %s", MakeHostName().c_str());

	const int wifiTimeoutSeconds = 120;
	WifiBusyTask wifiBusy(_display);
	_wifiManager.setConfigPortalTimeout(wifiTimeoutSeconds);
	while (WiFi.status() != WL_CONNECTED)
	{
		Logln("Try WIFI Connection");
		wifiBusy.StartCountDown(wifiTimeoutSeconds);
		_wifiManager.autoConnect(WiFi.getHostname(), AP_PASSWORD);
		// ESP.restart();
		// delay(1000);
	}

	// Connected
	_webPortal.Setup();
	Logln("Setup complete");
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
		// Update the loop performance counter
		_loopWaitTime = t;
		_display.SetLoopsPerSecond(_loopPersSecondCount, t);
		_loopPersSecondCount = 0;

		// Refresh RTK Display
		for (int i = 0; i < RTK_SERVERS; i++)
			_display.RefreshRtk(i);

		// Check memory pressure
		auto free = ESP.getFreeHeap();
		auto total = ESP.getHeapSize();

		auto temperature = _history.CheckTemperatureLoop();

		// Update the loop performance counter
		// Serial.printf("Loop %d G:%ld 1:%ld, 2:%ld 3:%ld Heap:%d%% %.1f°C\n",
		// 	_loopPersSecondCount,
		// 	_gpsParser.GetGpsBytesRec(),
		// 	_ntripServer0.GetPacketsSent(),
		// 	_ntripServer1.GetPacketsSent(),
		// 	_ntripServer2.GetPacketsSent(),
		// 	(int)(100.0 * free / total),
		// 	temperature);
	}

	// Check for push buttons
	if (IsButtonReleased(BUTTON_1, &_button1Current))
	{
		_lastButtonPress = t;
		Logln("Button 1");
		_display.NextPage();
	}
	if (IsButtonReleased(BUTTON_2, &_button2Current))
	{
		_lastButtonPress = t;
		Logln("Button 2");
		_display.ActionButton();
	}

	// Check if we should turn off the display
	// .. Note : This only work when powered from the GPS unit. WIth ESP32 powered from USB display is always on
#if T_DISPLAY_S3 == true
	digitalWrite(DISPLAY_POWER_PIN, ((t - _lastButtonPress) < 30000) ? HIGH : LOW);
#endif

	// Check for new data GPS serial data
	if (IsWifiConnected())
	{
		_display.SetGpsConnected(_gpsParser.ReadDataFromSerial(Serial2));
		_webPortal.Loop();
	}
	else
	{
		_display.SetGpsConnected(false);
	}

	// Update animations
	_display.Animate();
}

//////////////////////////////////////////////////////////////////////////////
// Read the base location from the disk
void LoadBaseLocation()
{
	std::string text;
	if (_myFiles.ReadFile(BASE_LOCATION_FILENAME, text))
	{
		Logln(StringPrintf(" - Read config '%s'", text.c_str()).c_str());
		_baseLocation = text;
	}
	else
	{
		_baseLocation = "";
		Logln(StringPrintf(" - E742 - Cannot read saved Server setting '%s'", BASE_LOCATION_FILENAME).c_str());
	}
}
void SaveBaseLocation(std::string newBaseLocation)
{
	_baseLocation = newBaseLocation;
	_myFiles.WriteFile(BASE_LOCATION_FILENAME, newBaseLocation.c_str());
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
		Logf("Wifi Status %d %s", status, WifiStatus(status));
		//	_display.SetWebStatus(status);
		_display.RefreshWiFiState();

		if (status == WL_CONNECTED)
		{
			// Setup the access point to prevend device getting stuck on a nearby network
			_wifiManager.setConfigPortalTimeout(0);
			auto res = _wifiManager.startConfigPortal(WiFi.getHostname(), AP_PASSWORD);
			if (!res)
				Logln("Failed to start config Portal (Maybe cos non-blocked)");
			else
				Logln("Config portal started");
		}
	}

	if (status == WL_CONNECTED)
		return true;

	// Start the connection process
	// Logln("E310 - No WIFI");
	unsigned long t = millis();
	unsigned long tDelta = t - _wifiFullResetTime;
	if (tDelta < WIFI_STARTUP_TIMEOUT)
	{
		return false;
	}

	Logln("Try resetting WIfi");
	delay(1000);

	// Reset the WIFI
	//_wifiFullResetTime = t;
	// We will block here until the WIFI is connected
	//_wifiManager.autoConnect(WiFi.getHostname(), "JohnIs#1");
	// WiFi.mode(WIFI_STA);
	// wl_status_t beginState = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	// Logf("WiFi Connecting %d %s\r\n", beginState, WifiStatus(beginState));
	//_display.RefreshWiFiState();

	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Maker a unique host name based on the MAC address with Rtk prefix
String MakeHostName()
{
	auto mac = WiFi.macAddress();
	mac.replace(":", "");
	return "Rtk_" + mac;
}

//  Check the Correct TFT Display Type is Selected in the User_Setup.h file
#if USER_SETUP_ID != 206 && USER_SETUP_ID != 25
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#error "Error! Please make sure the required LILYGO PCB in .pio\libdeps\lilygo-t-display\TFT_eSPI\User_Setup_Select.h See top of main.cpp for details"
#endif