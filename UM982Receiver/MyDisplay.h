#pragma once

// Handy RGB565 colour picker at https://chrishewett.com/blog/true-rgb565-colour-picker/
// IMage converter for sprite http://www.rinkydinkelectronics.com/t_imageconverter565.php

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
		//_img = TFT_eSprite(&_tft);
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
		const uint16_t COLORS[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_BLACK, TFT_WHITE };
		const int COLORS_SIZE = 5;

		// Draw the dynamic frame around the titlebar. First loop is grren second is red
		int t = (_animationAngle++ / 20) % (COLORS_SIZE * CIRF);
		uint16_t clr = COLORS[t/CIRF];

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
	void SetPosition(double lng, double lat)
	{
		if( _currentPage == 1)
		{
			SetValueFormatted(1, lng, &_lng, StringPrintf("%.9lf", lng), 3, 24, 236, 4);
			SetValueFormatted(1, lat, &_lat, StringPrintf("%.9lf", lat), 3, 52, 236, 4);
		}
		if (_currentPage == 2)
		{
			// Calculate the
			if (_lng != lng || _lat != lat)
			{
				double metres = HaversineMetres(lat, lng, _latSaved, _lngSaved);
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
	}

	void SetFixMode(int8_t n)
	{
		if (_fixMode == n)
			return;
		_fixMode = n;

		if (_currentPage != 1)
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

		if (_currentPage != 1)
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
		SetValue(3, status, &_webStatus, 2, 24, 236, 4);
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
	// Saver location if necessary
	void ToggleDeltaMode()
	{
		if( _currentPage != 2 )
			return;
		_lngSaved = _lng;
		_latSaved = _lat;
		Serial.printf("Save X,Y %.9lf, %.9lf\r\n", _lng, _lat);
		RefreshScreen();
	}

	///////////////////////////////////////////////////////////////////////////
	// Move to the next page
	void NextPage()
	{
		_currentPage++;
		if (_currentPage > 3)
			_currentPage = 0;
		RefreshScreen();
	}

	///////////////////////////////////////////////////////////////////////////
	// Draw everything back on the screen
	void RefreshScreen()
	{
		const char* title = "Unknown";
		_tft.setTextDatum(TL_DATUM);
		_tft.setTextColor(TFT_BLACK, TFT_WHITE, true);
		// Clear the working area
		switch (_currentPage)
		{
			case 0:
				_tft.fillScreen(0x8610);
				title = "Status";
				_tft.drawString("Version", 5, 50, 4);
				_tft.drawString(APP_VERSION, 105, 50, 4);
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
		double lng = _lng,
			   lat = _lat;
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
	template<typename T>
	void SetValue(int page, T n, T* pMember, int32_t x, int32_t y, int width, uint8_t font)
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

	template<typename T>
	void SetValueFormatted(int page, T n, T* pMember, const std::string text, int32_t x, int32_t y, int width, uint8_t font)
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
	void DrawCell(const char* pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour = TFT_WHITE, uint16_t bgColour = TFT_BLACK)
	{
		// Fill background
		int height = _tft.fontHeight(font);
		_tft.fillRoundRect(x, y, width, height + 3, 5, bgColour);

		// Draw text
		_tft.setFreeFont(&FreeMono18pt7b);
		_tft.setTextColor(fgColour, bgColour, true);
		_tft.setTextDatum(MC_DATUM);
		_tft.drawString(pstr, x + width / 2, y + (height / 2) + 2, font);

		// Frame it
		_tft.drawRoundRect(x, y, width, height + 3, 5, TFT_YELLOW);
	}

  private:
	TFT_eSPI _tft = TFT_eSPI();	 // Invoke library, pins defined in User_Setup.h
	TFT_eSprite _background = TFT_eSprite(&_tft);
	TFT_eSprite _img = TFT_eSprite(&_tft);
	int _currentPage = 3;		  // Page we are currently displaying
	int8_t _fixMode = -1;		  // Fix mode for RTK
	int8_t _satellites = -1;	  // Number of satellites
	int16_t _gpsPacketCount = 0;  // Number of packets received
	int16_t _rtkPacketCount = 0;  // Number of packets received
	std::string _time;			  // GPS time in minutes and seconds
	double _lng;				  // Longitude
	double _lat;				  // ..
	double _lngSaved;			  // Longitude saved for deltas
	double _latSaved;			  // ..
	int _animationAngle;		  // Animated wheel
	int _loopsPerSecond;		  // How many loop occur per second
	std::string _webStatus;		  // Status of connection
};