#pragma once

//#include "Actions.h"
#include "ActionsPrivate.h"

/// <summary>
/// Build the sequence looking for sequence
/// </summary>

/// @brief Class to catch the sequence
class SequenceCatcher
{
  public:
	/// @brief Setup the function to send the string to the keyboard
	void SetSendKeyFunction(std::function<void(std::string)> funcSentToKeyboard)
	{
		_funcSendString = funcSentToKeyboard;
	}

	/// @brief Process the key buffer builder
	/// @param key Key pressed
	void ProcessKey(char key)
	{
		// Build buffer
		if (millis() - _lastKey > MAX_KEY_DELAY)
			_buildBuffer = std::string();
		_lastKey = millis();
		_buildBuffer += key;

		// Check buffers
		for (int n = 0; n < ActionListLength; n++)
		{
			Action action = ActionList[n];

			if (action.Command.compare(_buildBuffer) == 0)
			{
				_funcSendString(std::string(action.Backspace, '\b'));
				_funcSendString(action.Macro);
				_lastKey = 0;
			}
		}
	}
  private:
	/// @brief Maximum delay between key presses before the buffer is reset
	const int MAX_KEY_DELAY = 2500;

	/// @brief Last key press time
	unsigned long _lastKey = 0;

	/// @brief Buffer to build the sequence
	std::string _buildBuffer;

	/// @brief Function to send string to keyboard
	std::function<void(std::string)> _funcSendString;
};
