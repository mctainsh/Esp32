#include "hal/uart_ll.h"
#include <string>
#pragma once

#include "HandyString.h"

#include <TFT_eSPI.h>  // Graphics and font library for ST7735 driver chip
#include <SPI.h>

//#include "Free_Fonts.h" // Include the header file attached to this sketch
#define ROW4 (135 - 54)

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
		_tft.init();
		_tft.setRotation(3);
		_tft.fillScreen(TFT_BLUE);

		// The standard ADAFruit font still works as before
		_tft.setTextColor(TFT_BLACK);
		_tft.setCursor(12, 5);
		_tft.print(StringPrintf("X %d Y %d", _tft.width(), _tft.height()).c_str());
	}

	/////////////////////////////////////////////////////////////////////////////
	// Simple functions to update display
	void SetCount(int n)
	{
		SetValue(n, &_count, 170, 5, 64, 2);
	}
	void SetTime(const std::string t)
	{
		SetValue(t, &_time, 2, ROW4, 124, 4);
	}
	void SetWebStatus( std::string status)
	{
		SetValue(status, &_webStatus, 2, 3, 164, 2);
	}
	void SetPosition(double lng, double lat)
	{
		if (_deltaMode)
		{
			// Calculate the
			if (_lng != lng || _lat != lat)
			{
				double metres = HaversineMetres(_lat, _lng, _latSaved, _lngSaved);
				_lng = lng;
				_lat = lat;
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
		else
		{
			//SetValueFormatted(lng, &_lng, StringPrintf("%.9lf", lng), 3, 18, 236, 4);
			SetValueFormatted(lat, &_lat, StringPrintf("%.9lf", lat), 3, 52, 236, 4);
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	// Saver location if necessary
	void ToggleDeltaMode()
	{
		_deltaMode = !_deltaMode;
		_lngSaved = _lng;
		_latSaved = _lat;

		RefreshScreen();
	}

	///////////////////////////////////////////////////////////////////////////
	// Draw everything back on the screen
	void RefreshScreen()
	{
		// Clear the working area
		_tft.fillScreen(TFT_BLUE);

		// Redraw
		double lng = _lng, lat = _lat;
		SetPosition(lng + 10, lat + 10);
		SetPosition(lng, lat);

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
	// FIx mode indicates if we have a location and if itr is RTK corrected
	void SetFixMode(int8_t n)
	{
		if (_fixMode == n)
			return;
		_fixMode = n;
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

	/////////////////////////////////////////////////////////////////////////////
	void SetSatellites(int8_t n)
	{
		if (_satellites == n)
			return;
		_satellites = n;
		std::string text = " ";
		if (_satellites != -1)
			text = std::to_string(_satellites);
		DrawCell(text.c_str(), 240 - 38 - 72, ROW4, 68, 7);
	}

	void IncrementGpsPackets()
	{
		_gpsPacketCount++;
		if (_gpsPacketCount > 32000)
			_gpsPacketCount = 10000;
		DrawCell(std::to_string(_gpsPacketCount).c_str(), 2, ROW4 + 32, 60, 2);
	}

	void IncrementRtkPackets(int completePackets)
	{
		_rtkPacketCount+= completePackets;
		if (_rtkPacketCount > 32000)
			_rtkPacketCount = 10000;
		DrawCell(std::to_string(_rtkPacketCount).c_str(), 64, ROW4 + 32, 60, 2);
	}

	/////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void SetValue(T n, T* pMember, int32_t x, int32_t y, int width, uint8_t font)
	{
		if (*pMember == n)
			return;
		*pMember = n;

		std::ostringstream oss;
		oss << *pMember;
		DrawCell(oss.str().c_str(), x, y, width, font);
	}

	template<typename T>
	void SetValueFormatted(T n, T* pMember, const std::string text, int32_t x, int32_t y, int width, uint8_t font)
	{
		if (*pMember == n)
			return;
		*pMember = n;

		DrawCell(text.c_str(), x, y, width, font);
	}

	///////////////////////////////////////////////////////////////////////////
	// NOTE: Font 6 is only numbers
	void DrawCell(const char* pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour = TFT_WHITE, uint16_t bgColour = TFT_BLACK)
	{
		// Fill background
		int height = _tft.fontHeight(font);
		_tft.fillRoundRect(x, y - 2, width, height + 3, 5, bgColour);

		// Draw text
		_tft.setFreeFont(&FreeMono18pt7b);
		_tft.setTextColor(fgColour, bgColour, true);
		_tft.setTextDatum(MC_DATUM);
		_tft.drawString(pstr, x + width / 2, y + (height / 2), font);

		// Frame it
		_tft.drawRoundRect(x, y - 2, width, height + 3, 5, TFT_YELLOW);
	}

  private:

	TFT_eSPI _tft = TFT_eSPI();	  // Invoke library, pins defined in User_Setup.h
	int8_t _fixMode = -1;		  // Fix mode for RTK
	int8_t _satellites = -1;	  // Number of satellites
	int16_t _gpsPacketCount = 0;  // Number of packets received
	int16_t _rtkPacketCount = 0;  // Number of packets received
	std::string _time;			  // GPS time in minutes and seconds
	double _lng;				  // Longitude
	double _lat;				  // Latitude
	double _lngSaved;			  // Longitude  saved
	double _latSaved;			  // Latitude saved
	bool _deltaMode = false;	  // True if displaying delta
	int _count;
	std::string _webStatus;		// Status of connection
};