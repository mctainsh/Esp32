#include "esp32-hal.h"
#pragma once

#include <vector>
#include <string>

///////////////////////////////////////////////////////////////////////////////
// Holds a collection of commands
// Verifies when a response matches the current queue item,
//	.. the next item is sent
//  .. command responses look like $command,version,response: OK*24
class GpsCommandQueue
{
private:
	std::vector<std::string> _strings;
	int _timeSent = 0;
	bool _gotReset = false;
	int _resetCount = 0;
	MyDisplay& _display;

public:
	///////////////////////////////////////////////////////////////////////////
	// Constructor
	GpsCommandQueue(MyDisplay& display) : _display(display)
	{
	}

	// ////////////////////////////////////////////////////////////////////////
	// Check if the first item in the list matches the given string
	bool IsCommandResponse(const std::string &str)
	{
		if (_strings.empty())
			return false; // List is empty, no match

		std::string match = "$command,";
		match += _strings.front();
		match += ",response: OK*";

		if (str.compare(0, match.size(), match) != 0)
			return false;

		// Send next command
		_strings.erase(_strings.begin());
		SendTopCommand();
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// Check if the GPS receiver has reset itself and send all the commands
	// .. reset command looks like "$devicename,COM1*67"
	bool HasDeviceReset(const std::string &str)
	{
		const std::string match = "$devicename,COM";
		if (str.compare(0, match.size(), match) != 0)
			return false;

		Serial.println("E200 - Reset command received");
		_resetCount++;
		_display.SetRtkStatus(StringPrintf("GPS Reset %d", _resetCount));
		StartInitialiseProcess();
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// Check if the GPS receiver has reset itself and send all the commands
	void StartInitialiseProcess()
	{
		// Load the commands
		_strings.clear();
		if (_gotReset)
		{
			Serial.println("GPS Queue StartInitialiseProcess");
			_strings.push_back("version");
			_strings.push_back("config signalgroup 3 6");
			_strings.push_back("MODE ROVER");
			_strings.push_back("CONFIG RTK TIMEOUT 10");
			_strings.push_back("GNGGA 1");
			_strings.push_back("saveconfig");
		}
		else
		{
			Serial.println("GPS FRESET");
			_strings.push_back("freset");
			_gotReset = true;
		}

		SendTopCommand();
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Send the command and check set timeouts
	void SendTopCommand()
	{
		if (_strings.empty())
			return;

		Serial2.println(_strings.front().c_str());
		_timeSent = millis();
	}
};
