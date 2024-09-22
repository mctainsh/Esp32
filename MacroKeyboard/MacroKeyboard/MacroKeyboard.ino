#include <USB.h>
#include <USBHIDKeyboard.h>

#include "SequenceCatcher.h"

//////////////////////////////////////////////////////////////////////////////
// Built to make a tool to 
// Created
//		John McTainsh
// 		19 Sept 2024
//  This example code is in the public domain.
//	Built using the sample at
//	http://www.arduino.cc/en/Tutorial/KeyboardMessage
//////////////////////////////////////////////////////////////////////////////

// Define the number of rows and columns of the keypad
const byte ROWS = 4;
const byte COLS = 4;

// Assign pins to rows
// >>> 4 x 4
// byte _rowPins[ROWS] = { 1, 2, 4, 6 };
// byte _colPins[COLS] = { 8, 10, 13, 14 };

//  >>> 4 x 3 (_colPins[3] is N/C) 
byte _rowPins[ROWS] = { 10, 1, 2, 6 };
byte _colPins[COLS] = { 8, 13, 4 };

// Define the symbols on the buttons of the keypad
const char KEY_MATRIX[ROWS][COLS] = {
	{ '1', '2', '3', 'A' },
	{ '4', '5', '6', 'B' },
	{ '7', '8', '9', 'C' },
	{ '*', '0', '#', 'D' }
};

// Last state of PCB button
int _previousPcbBtnState = HIGH;

USBHIDKeyboard Keyboard;  // USB Keyboard output

SequenceCatcher Catcher;

// input pin for pushbutton on PCB. Used to send test message
const int PCB_BTN = 0;

////////////////////////////////////////////////////////////////////////////////////
// Setup the outputs
void setup()
{
	// Startup blink
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);
	delay(200);

	// Make the pushButton pin an input:
	pinMode(PCB_BTN, INPUT_PULLUP);

	// Initialize row pins as inputs with pull-up resistors
	for (byte i = 0; i < ROWS; i++)
		pinMode(_rowPins[i], INPUT_PULLUP);

	// Initialize column pins as outputs
	for (byte j = 0; j < COLS; j++)
	{
		pinMode(_colPins[j], OUTPUT);
		digitalWrite(_colPins[j], HIGH);
	}

	// initialize control over the keyboard:
	Keyboard.begin();
	USB.begin();

	// Ready blink
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);

	// Allow catcher to send to the keyboard
	Catcher.SetSendKeyFunction([](std::string str)
	{
		Keyboard.print(str.c_str());
	});
}

//////////////////////////////////////////////////////////////////////////
// Loop forever
void loop()
{
	// Handy testing function without keypad attached
	int pcbBtnState = digitalRead(PCB_BTN);
	if (pcbBtnState != _previousPcbBtnState)
	{
		if (pcbBtnState == LOW)
			Keyboard.print("You pressed TEST button on V1.0");
		else
			Keyboard.print("RELEASED");
		digitalWrite(LED_BUILTIN, !pcbBtnState);
		_previousPcbBtnState = pcbBtnState;
	}

	// Scan each column
	for (byte j = 0; j < COLS; j++)
	{
		// Set the current column to LOW
		digitalWrite(_colPins[j], LOW);

		// Check each row
		for (byte i = 0; i < ROWS; i++)
		{
			if (digitalRead(_rowPins[i]) == LOW)
			{
				// Debounce
				delay(50);
				if (digitalRead(_rowPins[i]) == LOW)
				{
					// Show LED while button pressed
					digitalWrite(LED_BUILTIN, HIGH);

					// Send key add add to key catcher and write to screen
					Keyboard.write(KEY_MATRIX[i][j]);
					Catcher.ProcessKey(KEY_MATRIX[i][j]);

					// Wait until the key is released
					while (digitalRead(_rowPins[i]) == LOW)
					{
					}

					// Turn off the LED when key released
					digitalWrite(LED_BUILTIN, LOW);
				}
			}
		}

		// Set the column back to HIGH
		digitalWrite(_colPins[j], HIGH);
	}
}