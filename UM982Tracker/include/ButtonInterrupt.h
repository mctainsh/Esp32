#pragma once
#include <esp32-hal-gpio.h>
#include <functional>

///////////////////////////////////////////////////////////////////////////////
// Button to process presses in the loop
class ButtonInterrupt
{
public:
	const uint8_t PIN;

private:
	volatile unsigned long _lastButtonTime = 0; // Last time the button press down started
	const unsigned long DEBOUNCE_TIME = 250;	// milliseconds
	volatile unsigned long _pressDuration = 0;	// How log the button was pressed for
	volatile bool _isPressed = false;			// Is the button currently pressed

public:
	// Interrupt routines
	ButtonInterrupt(uint8_t pin) : PIN(pin)
	{
		pinMode(PIN, INPUT_PULLUP);
	}

	///////////////////////////////////////////////////////////
	// Destructor
	~ButtonInterrupt()
	{
		detachInterrupt(PIN);
	}

	///////////////////////////////////////////////////////////
	// Is the button currently pressed
	const inline bool IsPressed() { return _isPressed; }

	///////////////////////////////////////////////////////////
	// Attach the interrupt to a static function
	// Usage:
	// _button1.AttachInterrupts([]() { _button1.OnFallingIsr(); }, []() { _button1.OnRaisingIsr(); });
	void AttachInterrupts(void (*fallingISR)(), void (*risingISR)())
	{
		attachInterrupt(PIN, fallingISR, FALLING);
		attachInterrupt(PIN, risingISR, RISING);
	}

	///////////////////////////////////////////////////////////
	// Has the button been pressed
	bool WasPressed()
	{
		if (_pressDuration > 0)
		{
			_pressDuration = 0;
			return true;
		}
		return false;
	}

	///////////////////////////////////////////////////////////
	// The interrupt service routine
	void OnFallingIsr()
	{
		_lastButtonTime = millis();
		_isPressed = true;
	}

	///////////////////////////////////////////////////////////
	// The interrupt service routine
	void OnRaisingIsr()
	{
		auto time = millis();
		auto duration = time - _lastButtonTime;
		if (duration > DEBOUNCE_TIME)
		{
			_pressDuration = duration;
			_lastButtonTime = time;
		}
		_isPressed = false;
	}
};