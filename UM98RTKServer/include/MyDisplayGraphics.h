#pragma once

// Handy RGB565 colour picker at https://chrishewett.com/blog/true-rgb565-colour-picker/
// Image converter for sprite http://www.rinkydinkelectronics.com/t_imageconverter565.php

#include "MyDisplay.h"
#include "HandyString.h"

#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>

// Box size
#define CW 20
#define CH 20
#define SPACE 23

class MyDisplayGraphics
{
public:
	MyDisplayGraphics(TFT_eSPI *pTft) : _pTft(pTft), _background(pTft), _img(pTft)
	{
	}

	/////////////////////////////////////////////////////////////////////////////
	// Setup this display
	void Setup()
	{
		_img.createSprite(3, 14);
		_background.createSprite(18, 18);
	}

	/////////////////////////////////////////////////////////////////////////////
	// Animate the rotating wheel
	void Animate()
	{
		_animationAngle++;
		if (_animationAngle % 10 == 0)
		{
			_background.fillSprite(TFT_BLACK);
			_img.fillSprite(TFT_ORANGE);			
			_img.pushRotated(&_background, 360 - (millis() / 5) %360);
			DrawRotatingColouredSnake(&_background);
			_background.pushSprite(0, 0);
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	// Draw a rotating coloured snake around the outside of the box
	void DrawRotatingColouredSnake(TFT_eSPI *pTft)
	{
		// Draw the colorful box
		const int W = 16;
		const int H = 16;
		const int CIRF = (2 * (W + H));
		const uint16_t COLORS[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE};
		const int COLORS_SIZE = 4;

		// Draw the dynamic frame around the titlebar. First loop is green second is red
		int t = (_animationAngle / 500) % (COLORS_SIZE * CIRF);
		uint16_t clr = TFT_GREEN;//COLORS[t / CIRF];

		t = t % CIRF;

		// Draw the lines
		if (t > W + H + W)
			pTft->drawLine(0, H - 1, 0, H - (t - W - H - W), clr);
		else if (t > W + H)
			pTft->drawLine(W, H - 1, W - (t - W - H), H - 1, clr);
		else if (t > W)
			pTft->drawLine(W - 1, 0, W - 1, t - W, clr);
		else
			pTft->drawLine(2, 0, t, 0, clr);
	}

	/////////////////////////////////////////////////////////////////////////////
	// Update the current state of the graphics
	void Refresh()
	{
	}

	/////////////////////////////////////////////////////////////////////////////
	// Wi-Fi Status indicator
	void SetWebStatus(wl_status_t status)
	{
		DrawBoxTick(240 - SPACE - CW, 0, status == wl_status_t::WL_CONNECTED);
	}
	
	/////////////////////////////////////////////////////////////////////////////
	// GPS connected status
	void SetGpsConnected(bool connected)
	{
		DrawBoxTick(240 - 2*SPACE - CW, 0, connected);
	}

	/////////////////////////////////////////////////////////////////////////////
	// RTK server connection indicator
	void SetRtkStatus(std::string status)
	{
		bool connected = StartsWith(status, "Conn");
		DrawBoxTick(240 - 3*SPACE - CW, 0, connected);
	}
	
	/////////////////////////////////////////////////////////////////////////////
	// Transmitting data indicator
	void SetTxStatus(int httpCode)
	{
		DrawBoxTick(240 - 4*SPACE - CW, 0, httpCode == 200);
	}

	/////////////////////////////////////////////////////////////////////////////
	// Wi-Fi Status indicator
	void DrawBoxTick(int x, int y, bool tick)
	{
		_pTft->fillRoundRect(x, y, CW, CH, 5, tick ? TFT_GREEN : TFT_RED);
		_pTft->drawRoundRect(x, y, CW, CH, 5, TFT_WHITE);
		const int G = 4;	// Gap from edge
		int x0, x1, y0, y1; // TL to BR (Left stroke of tick)
		int x2, y2, x3, y3; // BL to TR
		uint32_t clr;
		if (tick)
		{
			clr = TFT_BLACK;
			x0 = x + G;
			y0 = y + CH / 4 * 3 - G;
			x1 = x + CW / 2;
			y1 = y + CH - G;
			x2 = x1;
			y2 = y1;
			x3 = x + CW - G;
			y3 = y + G;

			//_pTft->drawLine(x+2, y+CH/2, x+CW/2, y+CH-2, TFT_BLACK);
			//_pTft->drawLine(x+CW/2, y+CH-2, x+CW-2, y+2, TFT_BLACK);
		}
		else
		{
			clr = TFT_WHITE;
			x0 = x + G;
			y0 = y + G;
			x1 = x + CW - G;
			y1 = y + CH - G;
			x2 = x + G;
			y2 = y + CH - G;
			x3 = x + CW - G;
			y3 = y + G;

			//_pTft->drawLine(x+2, y+2, x+CW-2, y+CH-2, TFT_WHITE);
			//_pTft->drawLine(x+2, y+CH-2, x+CW-2, y+2, TFT_WHITE);
		}
		_pTft->drawLine(x0 + 1, y0 - 1, x1 + 1, y1 - 1, clr);
		_pTft->drawLine(x2 - 1, y2 - 1, x3 - 1, y3 - 1, clr);
		_pTft->drawLine(x0, y0, x1, y1, clr);
		_pTft->drawLine(x2, y2, x3, y3, clr);
		_pTft->drawLine(x0 - 1, y0 + 1, x1 - 1, y1 + 1, clr);
		_pTft->drawLine(x2 + 1, y2 + 1, x3 + 1, y3 + 1, clr);
	}

private:
	TFT_eSPI *const _pTft;
	TFT_eSprite _background;
	TFT_eSprite _img;
	int _animationAngle; // Animated wheel
};