#include "HardwareSerial.h"
#pragma once

// Handy RGB565 colour picker at https://chrishewett.com/blog/true-rgb565-colour-picker/
// Image converter for sprite http://www.rinkydinkelectronics.com/t_imageconverter565.php

#include "HandyString.h"
#include "MyFiles.h"

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <iostream>
#include <sstream>
#include <string>

// #include "Free_Fonts.h" // Include the header file attached to this sketch
#define ROW4 (135 - 54)

// Font size 4 with 4 rows
#define R1F4 20
#define R2F4 50
#define R3F4 80
#define R4F4 110

// Columns
#define COL1 5
#define COL2_P0 60
#define COL2_P0_W 178

#define SAVE_LNG_LAT_FILE "/SavedLatLng.txt"

extern MyFiles _myFiles;

class MyDisplay
{
public:
	MyDisplay()
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
		_img.createSprite(10, 40);
		_background.createSprite(50, 50);
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
		const int W = 240;
		const int H = 19;
		const int CIRF = (2 * (W + H));
		const uint16_t COLORS[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_BLACK, TFT_WHITE};
		const int COLORS_SIZE = 5;

		// Draw the dynamic frame around the titlebar. First loop is green second is red
		int t = (_animationAngle++ / 20) % (COLORS_SIZE * CIRF);
		uint16_t clr = COLORS[t / CIRF];

		t = t % CIRF;

		// Draw the lines
		if (t > W + H + W)
			_tft.drawLine(0, H - 1, 0, H - (t - W - H - W), clr);
		else if (t > W + H)
			_tft.drawLine(W, H - 1, W - (t - W - H), H - 1, clr);
		else if (t > W)
			_tft.drawLine(W - 1, 0, W - 1, t - W, clr);
		else
			_tft.drawLine(2, 0, t, 0, clr);

		// Rotating compass only appears on page
		if (_currentPage != 0)
			return;
		if (_animationAngle % 10 == 0)
		{
			_background.fillSprite(TFT_BLACK);
			_img.fillSprite(TFT_BLACK);
			_img.drawWedgeLine(5, 0, 5, 20, 1, 5, TFT_RED);
			//_img.drawArc()
			_img.pushRotated(&_background, _animationAngle / 10);

			_background.pushSprite(190, 60);
		}
	}

	// ************************************************************************//
	// Page 1 - CurrentGPS
	// ************************************************************************//
	void SetPosition(double lng, double lat, double height)
	{
		if (_currentPage == 1)
		{
			SetValueFormatted(1, lng, &_lng, StringPrintf("%.9lf", lng), 3, 24, 236, 4);
			SetValueFormatted(1, lat, &_lat, StringPrintf("%.9lf", lat), 3, 52, 236, 4);
			SetValueFormatted(1, height, &_height, StringPrintf("%.3lf m", lat), 3, 110, 120, 4);
		}
		if (_currentPage == 2)
		{
			// Calculate the
			if (_lng != lng || _lat != lat)
			{
				_lng = lng;
				_lat = lat;
				_height = height;
				DrawCell(StringPrintf("%.3lf", _height - _heightSaved).c_str(), 3, ROW4, 128, 7, TFT_BLACK, TFT_SILVER);

				double metres = HaversineMetres(lat, lng, _latSaved, _lngSaved);
				uint16_t clr = TFT_GREEN;
				std::string text = StringPrintf("%.3lf", metres);
				if (text.length() > 7)
				{
					text = StringPrintf("%.0lf", metres);
					clr = TFT_YELLOW;
				}
				if (text.length() > 6)
					text = "---";
				if (lat != 0 && lng != 0)
					DrawCell(text.c_str(), 3, 24, 234, 7, TFT_BLACK, clr);
			}
		}
	}

	void SetFixMode(int8_t n)
	{
		if (_fixMode == n)
			return;
		_fixMode = n;

		if (_currentPage != 1 && _currentPage != 2)
			return;

		std::string text = " ";
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
		DrawCell(text.c_str(), 240 - 38, ROW4, 34, 7, fg, bg);
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
		DrawCell(text.c_str(), 240 - 38 - 72, ROW4, 68, 7);
	}

	// ************************************************************************//
	// Page 1 - Delta GPS
	// ************************************************************************//

	// ************************************************************************//
	// Page 3 - System detail
	// ************************************************************************//
	void SetWebStatus(std::string status)
	{
		if( _webStatus == status )
			return;
		_webStatus = status;
		if( _currentPage != 0)
			return;

		DrawML(status.c_str(), COL2_P0, R1F4, COL2_P0_W, 4);
	}

	void SetLoopsPerSecond(int n)
	{
		SetValue(3, n, &_loopsPerSecond, 2, 54, 236, 4);
	}

	void IncrementGpsPackets()
	{
		_gpsPacketCount++;
		if (_gpsPacketCount > 32000)
			_gpsPacketCount = 10000;
		if (_currentPage != 3)
			return;
		DrawCell(std::to_string(_gpsPacketCount).c_str(), 2, ROW4, 116, 4);
	}

	void IncrementRtkPackets(int completePackets)
	{
		_rtkPacketCount += completePackets;
		if (_rtkPacketCount > 32000)
			_rtkPacketCount = 10000;
		if (_currentPage != 3)
			return;
		DrawCell(std::to_string(_rtkPacketCount).c_str(), 122, ROW4, 116, 4);
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
				std::string saveText = StringPrintf("%.9lf %.9lf %.3lf", _lng, _lat, _height);
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
		if (_currentPage > 3)
			_currentPage = 0;
		Serial.printf("Switch to page %d\r\n", _currentPage);
		RefreshScreen();
	}

	///////////////////////////////////////////////////////////////////////////
	// Draw everything back on the screen
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
			title = "Status";

			DrawLabel("Wi-Fi", COL1, R1F4, 2);
			DrawLabel("GPS", COL1, R2F4, 2);
			DrawLabel("Transmitter", COL1, R3F4, 2);
			DrawLabel("Version", COL1, R4F4, 2);

			
			DrawML("CONN", COL2_P0, R2F4, COL2_P0_W, 4);
			DrawML("READY", COL2_P0, R3F4, COL2_P0_W, 4);
			DrawML(APP_VERSION, COL2_P0, R4F4, COL2_P0_W, 4);

			break;
		case 1:
			_tft.fillScreen(TFT_GREEN);
			title = "Location";
			break;
		case 2:
			_tft.fillScreen(TFT_RED);
			title = "Location offset";
			break;
		case 3:
			_tft.fillScreen(TFT_YELLOW);
			title = "System";
			break;
		}
		DrawCell(title, 0, 0, 240, 2);

		// Redraw
		double lng = _lng, lat = _lat, h = _height;
		SetPosition(lng + 10, lat + 10, h + 10);
		SetPosition(lng, lat, _height);

		int8_t n = _fixMode;
		SetFixMode(n + 1);
		SetFixMode(n);

		std::string s = _webStatus;
		SetWebStatus(_webStatus + "?");
		SetWebStatus(s);

		n = _satellites;
		SetSatellites(n + 1);
		SetSatellites(n);

		_gpsPacketCount--;
		IncrementGpsPackets();
		_rtkPacketCount--;
		IncrementRtkPackets(1);
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
		if( datum == ML_DATUM)
			_tft.drawString(pstr, x+2, y + (height / 2) + 2, font);
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

private:
	TFT_eSPI _tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h
	TFT_eSprite _background = TFT_eSprite(&_tft);
	TFT_eSprite _img = TFT_eSprite(&_tft);
	uint16_t _bg = 0x8610;		 // Background colour
	int _currentPage = 0;		 // Page we are currently displaying
	int8_t _fixMode = -1;		 // Fix mode for RTK
	int8_t _satellites = -1;	 // Number of satellites
	int16_t _gpsPacketCount = 0; // Number of packets received
	int16_t _rtkPacketCount = 0; // Number of packets received
	std::string _time;			 // GPS time in minutes and seconds
	double _lng;				 // Longitude
	double _lat;				 // ..
	double _height;				 // ..
	double _lngSaved;			 // Saved for deltas
	double _latSaved;			 // ..
	double _heightSaved;		 //
	int _animationAngle;		 // Animated wheel
	int _loopsPerSecond;		 // How many loop occur per second
	std::string _webStatus;		 // Status of connection
};