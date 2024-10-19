// Handy RGB565 colour picker at https://chrishewett.com/blog/true-rgb565-colour-picker/
// Image converter for sprite http://www.rinkydinkelectronics.com/t_imageconverter565.php

#include "MyDisplay.h"

#include "Global.h"
#include "HandyString.h"
#include "MyFiles.h"
#include "MyDisplayGraphics.h"
#include "GpsParser.h"

#include "HandyLog.h"

#include <iostream>
#include <sstream>
#include <string>
#include <TFT_eSPI.h>

#include <iostream>
#include <sstream>
#include <string>
#include <NTRIPServer.h>

// Font size 4 with 4 rows
#define R1F4 20
#define R2F4 50
#define R3F4 80
#define R4F4 110
#define R5F4 140

// Columns
#define COL1 5
#define COL2_P0 60
#define COL2_P0_W (TFT_HEIGHT - COL2_P0 - 5)

// Three column display on page 4
#define COL2_P4 60
#define COL3_P4 190
#define COL_W_P4 128

#define SAVE_LNG_LAT_FILE "/SavedLatLng.txt"

extern MyFiles _myFiles;
extern NTRIPServer _ntripServer0;
extern NTRIPServer _ntripServer1;
extern GpsParser _gpsParser;

////////////////////////////////////////////////////////////////////////
// Setup this display
void MyDisplay::Setup()
{
	// Setup screen
	Logln("TFT Init");
	_tft.init();
	_tft.setRotation(1);
	_graphics.Setup();
	RefreshScreen();
}

// ************************************************************************//
// Page 0
// ************************************************************************//
void MyDisplay::Animate()
{
	_graphics.Animate();
}

///////////////////////////////////////////////////////////////////////////
// Draw the GPS connected tick box
void MyDisplay::SetGpsConnected(bool connected)
{
	if (_gpsConnected == connected)
		return;
	_gpsConnected = connected;
	_graphics.SetGpsConnected(_gpsConnected);
}

// ************************************************************************//
// Page 3 - System detail
// ************************************************************************//
void MyDisplay::SetLoopsPerSecond(int n, uint32_t millis)
{
	// Loops per second
	SetValue(2, n, &_loopsPerSecond, 2, R1F4, 236, 4);

	// Uptime
	if (_currentPage == 2)
	{
		uint32_t t = millis / 1000;
		std::string uptime = StringPrintf(":%02d", t % 60);
		t /= 60;
		uptime = StringPrintf(":%02d", t % 60) + uptime;
		t /= 60;
		uptime = StringPrintf("%02d", t % 24) + uptime;
		t /= 24;
		uptime = StringPrintf("%dd ", t) + uptime;
		DrawCell(uptime.c_str(), 2, R5F4, 236, 4);
	}
}

void MyDisplay::ResetGps()
{
	_gpsResetCount++;
	_gpsPacketCount--;
	IncrementGpsPackets();
}
void MyDisplay::IncrementGpsPackets()
{
	_gpsPacketCount++;
	if (_gpsPacketCount > 32000)
		_gpsPacketCount = 10000;
	if (_currentPage != 2)
		return;
	DrawCell(StringPrintf("%d - %d", _gpsResetCount, _gpsPacketCount).c_str(), 22, R2F4, 216, 4);
}

/////////////////////////////////////////////////////////////////////////////
// Depends on page currently shown
void MyDisplay::ActionButton()
{
	switch (_currentPage)
	{
	// Save current location
	case 2:
		break;

		// Reset GPS
		// case 3:
		//	Serial2.println("freset");
		//	break;
	}

	RefreshScreen();
}

///////////////////////////////////////////////////////////////////////////
// Move to the next page
void MyDisplay::NextPage()
{
	_currentPage++;
	if (_currentPage > 5)
		_currentPage = 0;
	Logf("Switch to page %d\r\n", _currentPage);
	RefreshScreen();
}

void MyDisplay::RefreshWiFiState()
{
	auto status = WiFi.status();
	_graphics.SetWebStatus(status);
	if (_currentPage != 0)
		return;
	if (status == WL_CONNECTED)
		DrawML(WiFi.localIP().toString().c_str(), COL2_P0, R1F4, COL2_P0_W, 4);
	else
		DrawML(WifiStatus(status), COL2_P0, R1F4, COL2_P0_W, 4);
}

void MyDisplay::RefreshRtk(int index)
{
	auto pServer = index == 0 ? &_ntripServer0 : &_ntripServer1;
	_graphics.SetRtkStatus(index, pServer->GetStatus());

	if (_currentPage != 1)
		return;
	int col = index == 0 ? COL2_P4 : COL3_P4;

	DrawML(pServer->GetStatus(), col, R1F4, COL_W_P4, 4);
	DrawMR(std::to_string(pServer->GetReconnects()).c_str(), col, R2F4, COL_W_P4, 4);
	DrawMR(std::to_string(pServer->GetPacketsSent()).c_str(), col, R3F4, COL_W_P4, 4);
	DrawMR(pServer->AverageSendTime().c_str(), col, R4F4, COL_W_P4, 4);
}

void MyDisplay::RefreshRtkLog()
{
	if (_currentPage != 4 && _currentPage != 5)
		return;

	NTRIPServer *pServer = _currentPage == 4 ? &_ntripServer0 : &_ntripServer1;
	RefreshLog(pServer->GetLogHistory());
}
void MyDisplay::RefreshGpsLog()
{
	if (_currentPage != 3)
		return;
	RefreshLog(_gpsParser.GetLogHistory());
}
void MyDisplay::RefreshLog(const std::vector<std::string> &log)
{
	// Clear the background
	_tft.fillRect(0, R1F4, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);

	_tft.setCursor(0, R1F4);
	_tft.setTextColor(TFT_GREEN, TFT_BLACK);
	_tft.setTextDatum(TL_DATUM);
	_tft.setTextFont(1);

	for (auto it = log.rbegin(); it != log.rend(); ++it)
			_tft.println(it->c_str());	
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
void MyDisplay::RefreshScreen()
{
	const char *title = "Unknown";
	_tft.setTextDatum(TL_DATUM);
	// Clear the working area

	switch (_currentPage)
	{
	case 0:
		_bg = 0x07eb;
		_tft.fillScreen(_bg);
		title = "  Network";

		DrawLabel("Wi-Fi", COL1, R1F4, 2);
		DrawLabel("TYPE", COL1, R2F4, 2);
		DrawLabel("Firmware", COL1, R3F4, 2);
		DrawLabel("Serial", COL1, R4F4, 2);
		DrawLabel("Version", COL1, R5F4, 2);

		DrawML(_gpsParser.GetDeviceType().c_str(), COL2_P0, R2F4, COL2_P0_W, 4);
		DrawML(_gpsParser.GetDeviceFirmware().c_str(), COL2_P0, R3F4, COL2_P0_W, 4);
		DrawML(_gpsParser.GetDeviceSerial().c_str(), COL2_P0, R4F4, COL2_P0_W, 4);
		DrawML(APP_VERSION, COL2_P0, R5F4, COL2_P0_W, 4);

		break;
	case 1:
		_tft.fillScreen(TFT_ORANGE);
		title = "  RTK Server";
		DrawLabel("State", COL1, R1F4, 2);
		DrawLabel("Reconnects", COL1, R2F4, 2);
		DrawLabel("Sends", COL1, R3F4, 2);
		DrawLabel("us", COL1, R4F4, 2);
		break;
	case 2:
		_tft.fillScreen(TFT_RED);
		title = "  System State";
		break;
	case 3:
		_tft.fillScreen(TFT_BLACK);
		title = "  GPS Log";
		break;

	case 4:
		_tft.fillScreen(TFT_BLACK);
		title = StringPrintf("  0 - %s", _ntripServer0.GetAddress()).c_str();
		break;

	case 5:
		_tft.fillScreen(TFT_BLACK);
		title = StringPrintf("  1 - %s", _ntripServer1.GetAddress()).c_str();
		break;
	}
	DrawML(title, 20, 0, 200, 2);

	// Redraw
	_graphics.SetGpsConnected(_gpsConnected);

	_gpsPacketCount--;
	IncrementGpsPackets();

	RefreshWiFiState();

	RefreshRtk(0);
	RefreshRtk(1);
	RefreshGpsLog();
	RefreshRtkLog();
}

/////////////////////////////////////////////////////////////////////////////
template <typename T>
void MyDisplay::SetValue(int page, T n, T *pMember, int32_t x, int32_t y, int width, uint8_t font)
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
void MyDisplay::SetValueFormatted(int page, T n, T *pMember, const std::string text, int32_t x, int32_t y, int width, uint8_t font)
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
void MyDisplay::DrawCell(const char *pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour, uint16_t bgColour, uint8_t datum)
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
	else if (datum == MR_DATUM)
		_tft.drawString(pstr, x + width - 2, y + (height / 2) + 2, font);
	else
		_tft.drawString(pstr, x + width / 2, y + (height / 2) + 2, font);

	// Frame it
	_tft.drawRoundRect(x, y, width, height + 3, 5, TFT_YELLOW);
}
void MyDisplay::DrawML(const char *pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour, uint16_t bgColour)
{
	DrawCell(pstr, x, y, width, font, fgColour, bgColour, ML_DATUM);
}
void MyDisplay::DrawMR(const char *pstr, int32_t x, int32_t y, int width, uint8_t font, uint16_t fgColour, uint16_t bgColour)
{
	DrawCell(pstr, x, y, width, font, fgColour, bgColour, MR_DATUM);
}

///////////////////////////////////////////////////////////////////////////
// NOTE: Font 6 is only numbers
void MyDisplay::DrawLabel(const char *pstr, int32_t x, int32_t y, uint8_t font)
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
