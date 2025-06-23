#pragma once

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <WiFi.h>

#include "MyDisplayGraphics.h"

#define SAVE_LNG_LAT_FILE "/SavedLatLng.txt"

class MyDisplay
{
public:
	MyDisplay() {}

	void Setup();
	void Animate();
	void SetGpsConnected(bool connected);
	void DisplayTime(unsigned long mil);
	void SetPerformance(std::string performance);
	void UpdateGpsStarts(bool restart, bool reinitialize);
	void IncrementGpsPackets();
	void ActionButton();
	void NextPage();
	void RefreshWiFiState();
	void RefreshRtk(int index);
	void RefreshLog(const std::vector<std::string> &log);
	void RefreshGpsLog();
	void RefreshRtkLog();
	void RefreshScreen();

	/////////////////////////////////////////////////////////////////////////////
	template <typename T>
	void SetValue(int page, T n, T *pMember, int32_t x, int32_t y, int width, uint8_t font);
	template <typename T>
	void SetValueFormatted(int page, T n, T *pMember, const std::string text, int32_t x, int32_t y, int width, uint8_t font);
	void DrawKeyLine(int y, int tick);
	void DrawCell(const char *pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour = TFT_WHITE, uint16_t bgColour = TFT_BLACK, uint8_t datum = MC_DATUM);
	void SetCell(std::string text, int page, int row, uint8_t datum = MC_DATUM);
	void DrawML(const char *pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour = TFT_WHITE, uint16_t bgColour = TFT_BLACK);
	void DrawMR(const char *pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour = TFT_WHITE, uint16_t bgColour = TFT_BLACK);
	void DrawLabel(const char *pstr, int32_t x, int32_t y, uint8_t font);

	inline void GetGpsStats(int32_t &resetCount, int32_t &reinitialize, int32_t &messageCount) const
	{
		resetCount = _gpsResetCount;
		reinitialize = _gpsReinitialize;
		messageCount = _gpsMsgCount;
	}

private:
	TFT_eSPI _tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h
	MyDisplayGraphics _graphics = MyDisplayGraphics(&_tft);
	uint16_t _bg = 0x8610;		  // Background colour
	uint16_t _fg = TFT_WHITE;	  // Foreground for labels
	int _currentPage = 0;		  // Page we are currently displaying
	bool _gpsConnected;			  // GPS connected
	int32_t _gpsResetCount = 0;	  // Number GPS resets
	int32_t _gpsReinitialize = 0; // Number GPS initializations
	int32_t _gpsMsgCount = 0;	  // Number GPS of packets received
	int _sendGood = 0;			  // Number of good sends
	int _sendBad = 0;			  // Number of bad sends
	int _httpCode = 0;			  // Last HTTP code
	std::string _time;			  // GPS time in minutes and seconds
	int _animationAngle;		  // Animated wheel
	std::string _performance;	  // Performance of WIFI, temperature and others
};