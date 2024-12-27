#pragma once

// Handy RGB565 colour picker at https://chrishewett.com/blog/true-rgb565-colour-picker/
// Image converter for sprite http://www.rinkydinkelectronics.com/t_imageconverter565.php

#include "HardwareSerial.h"
#include "Global.h"
#include "HandyString.h"
#include "MyFiles.h"
#include "MyDisplayGraphics.h"
#include "CredentialPrivate.h"
// #include "NTRIPClient.h"

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <iostream>
#include <sstream>
#include <string>

// #include "Free_Fonts.h" // Include the header file attached to this sketch
#if T_DISPLAY_S3
#define ROW2 (40)
#define ROW4 (169 - 54)
#else
#define ROW2 (24)
#define ROW4 (84)
#endif

// Font size 4 with 4 rows
#define R1F4 20
#define R2F4 50
#define R3F4 80
#define R4F4 110

// Columns
#define COL1 5
#define COL2_P0 60
#define COL2_P0_W SCR_W - 5 - COL2_P0

#define SAVE_LNG_LAT_FILE "/SavedLatLng.txt"

extern MyFiles _myFiles;
// extern NTRIPClient _ntripClient;

class MyDisplay
{
private:
	TFT_eSPI _tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h
	MyDisplayGraphics _graphics = MyDisplayGraphics(&_tft);
	uint16_t _bg = 0x8610;		 // Background colour
	int _currentPage = 0;		 // Page we are currently displaying
	bool _gpsConnected;			 // GPS connected
	int8_t _fixMode = -1;		 // Fix mode for RTK
	int8_t _satellites = -1;	 // Number of satellites
	int16_t _gpsResetCount = 0;	 // Number GPS resets
	int16_t _gpsPacketCount = 0; // Number GPS of packets received
	int16_t _rtkResetCount = 0;	 // Number RTK restarts
	int16_t _rtkPacketCount = 0; // Number of packets received
	int _sendGood = 0;			 // Number of good sends
	int _sendBad = 0;			 // Number of bad sends
	int _httpCode = 0;			 // Last HTTP code
	std::string _time;			 // GPS time in minutes and seconds
	double _lng;				 // Longitude
	double _lat;				 // ..
	double _height;				 // ..
	double _lngSaved;			 // Saved for deltas
	double _latSaved;			 // ..
	double _heightSaved;		 //
	int _animationAngle;		 // Animated wheel
	int _loopsPerSecond;		 // How many loop occur per second
	wl_status_t _webStatus;		 // WIFI Status of connection to hot spot
	std::string _webRtkStatus;	 // RTK connection status
	std::string _webTxStatus;	 // Transmitting of data status
	const bool _isTDisplayS3 = T_DISPLAY_S3;

	// Callback functions
	const std::function<void()> _onShowNtripSettings;

public:
	MyDisplay(const std::function<void()> onShowNtripSettings) : _onShowNtripSettings(onShowNtripSettings)
	{
	}

	/////////////////////////////////////////////////////////////////////////////
	// Setup this display
	void Setup()
	{
		// Load last lat long
		std::string llText;
		if (_myFiles.ReadFile(SAVE_LNG_LAT_FILE, llText))
		{
			Serial.printf("\tRead LL '%s'\r\n", llText.c_str());
			std::stringstream ss(llText);
			ss >> _lngSaved >> _latSaved >> _heightSaved;
			if (ss.fail())
				Serial.println("\tE341 - Cannot read saved Lng Lat");
			else
				Serial.printf("\tRecovered LL %.9lf,%.9lf, %.3lf m\r\n", _lngSaved, _latSaved, _heightSaved);
		}

		// Setup screen
		_tft.init();
		_tft.setRotation(3);
		_graphics.Setup();
		RefreshScreen();
	}

	// Do we currently have a location
	bool HasLocation() const
	{
		return _lat != 0.0 && _lng != 0.0;
	}

	// ************************************************************************//
	// Page 0
	// ************************************************************************//
	void Animate()
	{
		_graphics.Animate();
	}

	// ************************************************************************//
	// Page 1 - CurrentGPS
	// ************************************************************************//
	void SetPosition(double lngNew, double latNew, double height)
	{
		int e2 = SCR_W - 4 - 74; // End just before N0-. Satellites
		if (_currentPage == 1)
		{
			SetValueFormatted(1, lngNew, &_lng, StringPrintf("%.9lf", lngNew), 3, R1F4, e2, 4);
			SetValueFormatted(1, latNew, &_lat, StringPrintf("%.9lf", latNew), 3, R2F4, e2, 4);
			SetValueFormatted(1, height, &_height, StringPrintf("%.3lf m", latNew), 3, R3F4, 120, 4);
		}

		// Save new value
		if (_lng == lngNew || _lat == latNew)
			return;
		_lng = lngNew;
		_lat = latNew;
		_height = height;

		if (_currentPage == 2)
		{
			int maxDistLength = 7;
			// Calculate the
			auto heightDiff = StringPrintf("%.3lf", _height - _heightSaved).substr(0, 6);

			if (!_isTDisplayS3)
			{
				heightDiff = heightDiff.substr(0, 4);
				maxDistLength = 5;
			}

			DrawCell(heightDiff.c_str(), 3, ROW4, SCR_W - 146, 7, TFT_BLACK, TFT_SILVER);

			// Calculate the bearing
			double bearing = CalculateBearing(_latSaved, _lngSaved, _lat, _lng);
			auto bearingText = StringPrintf("%.0lf", bearing);
			DrawCell(bearingText.c_str(), SCR_W - 140, ROW4, 100, 7, TFT_BLACK, TFT_VIOLET);

			// Calculate the distance
			double metres = HaversineMetres(_lat, _lng, _latSaved, _lngSaved);
			uint16_t clr = TFT_GREEN;
			auto text = StringPrintf("%.3lf", metres);
			if (text.length() > maxDistLength)
			{
				text = StringPrintf("%.0lf", metres);
				clr = TFT_YELLOW;
			}
			if (text.length() > maxDistLength)
				text = "---";
			if (_lat == 0 || _lng == 0)
				text = "No GPS";
			DrawCell(text.c_str(), 3, ROW2, e2, 7, TFT_BLACK, clr);
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Calculate the distance between two points in metres
	double CalculateBearing(double lat1, double lon1, double lat2, double lon2)
	{
		// Convert to radians
		lat1 = lat1 * M_PI / 180.0;
		lon1 = lon1 * M_PI / 180.0;
		lat2 = lat2 * M_PI / 180.0;
		lon2 = lon2 * M_PI / 180.0;

		// Calculate the bearing
		double y = sin(lon2 - lon1) * cos(lat2);
		double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon2 - lon1);
		double bearing = atan2(y, x) * 180.0 / M_PI;
		if (bearing < 0)
			bearing += 360.0;
		return bearing;
	}

	///////////////////////////////////////////////////////////////////////////
	// Draw the GPS connected tick box
	void SetGpsConnected(bool connected)
	{
		if (_gpsConnected == connected)
			return;
		_gpsConnected = connected;
		_graphics.SetGpsConnected(_gpsConnected);
	}

	///////////////////////////////////////////////////////////////////////////
	// Fix mode appears big ON GPS pages and tiny on all pages
	void SetFixMode(int8_t n)
	{
		if (_fixMode == n)
			return;
		_fixMode = n;

		std::string text = "-";
		if (_fixMode != -1)
			text = std::to_string(_fixMode);

		uint16_t fg = TFT_WHITE;
		uint16_t bg = TFT_BLACK;
		if (n < 1)
		{
			fg = TFT_BLACK;
			bg = TFT_RED;
		}
		else if (n == 1)
		{
			fg = TFT_WHITE;
			bg = TFT_RED;
		}
		else if (n == 2)
		{
			fg = TFT_BLACK;
			bg = TFT_YELLOW;
		}
		else if (n > 2)
		{
			fg = TFT_BLACK;
			bg = TFT_GREEN;
		}
		DrawCell(text.c_str(), SCR_W - 20, 0, 20, 2, fg, bg);

		if (_currentPage == 1 || _currentPage == 2)
			DrawCell(text.c_str(), SCR_W - 38, ROW4, 34, 7, fg, bg);
	}

	void SetTime(const std::string t)
	{
		SetValue(1, t, &_time, 2, ROW4, 124, 4);
	}

	void SetSatellites(int8_t n)
	{
		if (_satellites == n)
			return;
		_satellites = n;

		if (_currentPage != 1 && _currentPage != 2)
			return;

		std::string text = " ";
		if (_satellites != -1)
			text = std::to_string(_satellites);
		DrawCell(text.c_str(), SCR_W - 72, ROW2, 68, 7);
	}

	// ************************************************************************//
	// Page 1 - Delta GPS
	// ************************************************************************//

	// ************************************************************************//
	// Page 3 - System detail
	// ************************************************************************//
	void RefreshWiFiState()
	{
		auto status = WiFi.status();
		_graphics.SetWebStatus(status);
		if (_currentPage == 0)
		{
			DrawML(WifiStatus(status), COL2_P0, R1F4, COL2_P0_W, 4);
			if (status != WL_CONNECTED)
			//{
			//	DrawML(WiFi.localIP().toString().c_str(), COL2_P0, R1F4, COL2_P0_W, 4);
			//}
			//else
			{
				
				DrawML("X-192.168.4.1", COL2_P0, R3F4, COL2_P0_W, 4);
				DrawML(WiFi.getHostname(), COL2_P0, R4F4, COL2_P0_W, 4);
			}
		}
		if( _currentPage == 5)
		{
			DrawML(WiFi.localIP().toString().c_str(), COL2_P0, R1F4, COL2_P0_W, 4);
			DrawML(WiFi.SSID().c_str(), COL2_P0, R2F4, COL2_P0_W, 4);
			DrawML(WiFi.getHostname(), COL2_P0, R3F4, COL2_P0_W, 4);
			DrawML(WiFi.macAddress().c_str(), COL2_P0, R4F4, COL2_P0_W, 4);
		}
	}

	void SetRtkStatus(std::string status)
	{
		if (_webRtkStatus == status)
			return;
		_webRtkStatus = status;
		_graphics.SetRtkStatus(status);
		if (_currentPage != 0)
			return;
		DrawML(status.c_str(), COL2_P0, R2F4, COL2_P0_W, 4);
	}

	void SetLoopsPerSecond(int n)
	{
		SetValue(3, n, &_loopsPerSecond, 2, R2F4, SCR_W - 4, 4);
	}

	void ResetGps()
	{
		_gpsResetCount++;
		_gpsPacketCount--;
		IncrementGpsPackets();
	}
	void IncrementGpsPackets()
	{
		_gpsPacketCount++;
		if (_gpsPacketCount > 32000)
			_gpsPacketCount = 10000;
		if (_currentPage != 3)
			return;
		DrawCell(StringPrintf("%d - %d", _gpsResetCount, _gpsPacketCount).c_str(), 2, ROW4, 116, 4);
	}

	void ResetRtk()
	{
		_rtkResetCount++;
		IncrementRtkPackets(0);
	}
	void IncrementRtkPackets(int completePackets)
	{
		_rtkPacketCount += completePackets;
		if (_rtkPacketCount > 32000)
			_rtkPacketCount = 10000;
		if (_currentPage != 3)
			return;
		_graphics.SetRtkStatus("Connected");
		DrawCell(StringPrintf("%d - %d", _rtkResetCount, _rtkPacketCount).c_str(), 122, ROW4, 116, 4);
	}

	void IncrementSendGood(int httpCode)
	{
		_sendGood++;
		DrawSendStats(httpCode);
	}

	void IncrementSendBad(int httpCode)
	{
		_sendBad++;
		DrawSendStats(httpCode);
	}
	void DrawSendStats(int httpCode)
	{
		_httpCode = httpCode;
		_graphics.SetTxStatus(httpCode);
		if (_currentPage == 3)
			DrawCell(StringPrintf("Sends G:%d B:%d", _sendGood, _sendBad).c_str(), COL2_P0, R1F4, COL2_P0_W, 4);
		if (_currentPage == 0)
		{
			auto status = StringPrintf("E:%d", _httpCode);
			if (_httpCode == 200)
				status = "OK";
			DrawML(status.c_str(), COL2_P0, R3F4, COL2_P0_W, 4);
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	// Depends on page currently shown
	void ActionButton()
	{
		switch (_currentPage)
		{
		// Save current location
		case 2:
			if (!HasLocation())
				return;
			_lngSaved = _lng;
			_latSaved = _lat;
			_heightSaved = _height;
			{
				auto saveText = StringPrintf("%.9lf %.9lf %.3lf", _lng, _lat, _height);
				Serial.printf("Saving X,Y %s\r\n", saveText.c_str());
				_myFiles.WriteFile(SAVE_LNG_LAT_FILE, saveText.c_str());
			}
			break;

		// Reset GPS
		case 3:
			Serial2.println("freset");
			break;
		}

		RefreshScreen();
	}

	///////////////////////////////////////////////////////////////////////////
	// Move to the next page
	void NextPage()
	{
		_currentPage++;
		if (_currentPage > 6)
			_currentPage = 0;
		Serial.printf("Switch to page %d\r\n", _currentPage);
		RefreshScreen();
	}

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	// Draw everything back on the screen
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	void RefreshScreen()
	{
		const char *title = "Unknown";
		_tft.setTextDatum(TL_DATUM);
		// Clear the working area

		switch (_currentPage)
		{
		case 0:
			_bg = 0x07eb;
			_tft.fillScreen(_bg);
			title = "  0 - Network";

			DrawLabel("Wi-Fi", COL1, R1F4, 2);
			DrawLabel("RTK", COL1, R2F4, 2);
			DrawLabel("TX", COL1, R3F4, 2);
			DrawLabel("Version", COL1, R4F4, 2);

			DrawML(APP_VERSION, COL2_P0, R4F4, COL2_P0_W, 4);

			break;
		case 1:
			_tft.fillScreen(TFT_GREEN);
			title = "  1 - Location";
			break;
		case 2:
			_bg = 0x0961;
			_tft.fillScreen(_bg);
			title = "  2 - Location offset";
			if (_isTDisplayS3)
			{
				_tft.setTextColor(TFT_WHITE, _bg, false);
				_tft.setTextDatum(BL_DATUM);
				_tft.drawString("Distance (m)", 10, ROW2 - 2, 2);
				_tft.drawString("#Sats", SCR_W - 50, ROW2 - 2, 2);
				_tft.drawString("Height (m)", 10, ROW4 - 2, 2);
				_tft.drawString("Bearing°", SCR_W - 130, ROW4 - 2, 2);
				_tft.drawString("FIX", SCR_W - 30, ROW4 - 2, 2);
			}
			break;
		case 3:
			_tft.fillScreen(TFT_YELLOW);
			title = "  3 - System";
			break;

		case 4:
			_bg = 0x07eb;
			_tft.fillScreen(_bg);
			title = "  4 - Settings";

			_onShowNtripSettings();
			break;
		case 5:
			_bg = 0xd345;
			_tft.fillScreen(_bg);
			title = "  5 - WIFI";
			DrawLabel("IP", COL1, R1F4, 2);
			DrawLabel("SSID", COL1, R2F4, 2);
			DrawLabel("Host", COL1, R3F4, 2);
			DrawLabel("Mac", COL1, R4F4, 2);
			break;
		case 6:
			_bg = 0xa51f;
			_tft.fillScreen(_bg);
			title = "  6 - Key";
			DrawLabel("Wi-Fi", COL1, R1F4, 2);
			DrawLabel("GPS", COL1, R2F4, 2);
			DrawLabel("RTK", COL1, R3F4, 2);
			DrawLabel("TX", COL1, R4F4, 2);
			DrawKeyLine(R1F4, 4);
			DrawKeyLine(R2F4, 3);
			DrawKeyLine(R3F4, 2);
			DrawKeyLine(R4F4, 1);
		}
		DrawML(title, 20, 0, 200, 2);

		// Redraw
		double lng = _lng, lat = _lat, h = _height;
		SetPosition(lng + 10, lat + 10, h + 10);
		SetPosition(lng, lat, _height);

		_graphics.SetGpsConnected(_gpsConnected);
		int8_t n = _fixMode;
		SetFixMode(n + 1);
		SetFixMode(n);

		RefreshWiFiState();

		auto s = _webRtkStatus;
		SetRtkStatus(_webRtkStatus + "?");
		SetRtkStatus(s);

		n = _satellites;
		SetSatellites(n + 1);
		SetSatellites(n);

		_gpsPacketCount--;
		IncrementGpsPackets();
		_rtkPacketCount--;
		IncrementRtkPackets(1);

		DrawSendStats(_httpCode + 1);
		DrawSendStats(_httpCode);
	}

	///////////////////////////////////////////////////////////////////////////
	// Draw line from text to top row tick position
	void DrawKeyLine(int y, int tick)
	{
		y += 10;
		int box = SCR_W - (SPACE * tick) - CW / 2;
		_tft.drawLine(50, y, box, y, TFT_BLACK);
		_tft.drawLine(box, y, box, CH, TFT_BLACK);
	}

	/////////////////////////////////////////////////////////////////////////////
	template <typename T>
	void SetValue(int page, T n, T *pMember, int32_t x, int32_t y, int width, uint8_t font)
	{
		if (*pMember == n)
			return;
		*pMember = n;

		if (page != _currentPage)
			return;
		std::ostringstream oss;
		oss << *pMember;
		DrawCell(oss.str().c_str(), x, y, width, font);
	}

	template <typename T>
	void SetValueFormatted(int page, T n, T *pMember, const std::string text, int32_t x, int32_t y, int width, uint8_t font)
	{
		if (*pMember == n)
			return;
		*pMember = n;
		if (_currentPage != page)
			return;
		DrawCell(text.c_str(), x, y, width, font);
	}

	///////////////////////////////////////////////////////////////////////////
	// NOTE: Font 6 is only numbers
	void DrawCell(const char *pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour = TFT_WHITE, uint16_t bgColour = TFT_BLACK, uint8_t datum = MC_DATUM)
	{
		// Fill background
		int height = _tft.fontHeight(font);
		_tft.fillRoundRect(x, y, width, height + 3, 5, bgColour);

		// Draw text
		_tft.setFreeFont(&FreeMono18pt7b);
		_tft.setTextColor(fgColour, bgColour, true);
		_tft.setTextDatum(datum);
		if (datum == ML_DATUM)
			_tft.drawString(pstr, x + 2, y + (height / 2) + 2, font);
		else
			_tft.drawString(pstr, x + width / 2, y + (height / 2) + 2, font);

		// Frame it
		_tft.drawRoundRect(x, y, width, height + 3, 5, TFT_YELLOW);
	}
	void DrawML(const char *pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour = TFT_WHITE, uint16_t bgColour = TFT_BLACK)
	{
		DrawCell(pstr, x, y, width, font, fgColour, bgColour, ML_DATUM);
	}

	///////////////////////////////////////////////////////////////////////////
	// NOTE: Font 6 is only numbers
	void DrawLabel(const char *pstr, int32_t x, int32_t y, uint8_t font)
	{
		// Fill background
		int height = _tft.fontHeight(font);
		//_tft.fillRoundRect(x, y, width, height + 3, 5, bgColour);

		// Draw text
		_tft.setFreeFont(&FreeMono18pt7b);
		_tft.setTextColor(TFT_BLACK, _bg, true);
		_tft.setTextDatum(ML_DATUM);
		_tft.drawString(pstr, x, y + (height / 2) + 2, font);

		// Frame it
		//_tft.drawRoundRect(x, y, width, height + 3, 5, TFT_YELLOW);
	}
};