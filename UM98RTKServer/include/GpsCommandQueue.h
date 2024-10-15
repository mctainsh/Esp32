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
public:
	GpsCommandQueue()
	{
		StartInitialiseProcess();
	}

	bool StartupComplete() const
	{
		return _startupComplete;
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
		if( _gotReset && _strings.empty())
		{
			Serial.printf("GPS Startup Commands Complete\r\n");
			_startupComplete = true;
		}
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
		_gotReset = true;
		StartInitialiseProcess();
		return true;
	}


	///////////////////////////////////////////////////////////////////////////
	// Start the initialise process by filling the queue
	void StartInitialiseProcess()
	{
		_startupComplete = false;
		// Load the commands
		Serial.printf("GPS Queue StartInitialiseProcess %d\r\n", _gotReset);
		_strings.clear();
		if (_gotReset)
		{
			_strings.push_back("CONFIG SIGNALGROUP 3 6"); //(for UM982)
			_strings.push_back("mode base time 60 2 2.5");
			_strings.push_back("rtcm1005 30");
			_strings.push_back("rtcm1033 30");
			_strings.push_back("rtcm1077 1");
			_strings.push_back("rtcm1087 1");
			_strings.push_back("rtcm1097 1");
			_strings.push_back("rtcm1117 1");
			_strings.push_back("rtcm1127 1");
			_strings.push_back("saveconfig");
		}
		else
		{
			_strings.push_back("freset");
		}

		SendTopCommand();
	}

	//////////////////////////////////////////////////////////////////////////
	// Check queue for timeouts
	void CheckForTimeouts()
	{
		//Serial.printf("Check TO %d,  %d\r\n", _strings.size(), (millis() - _timeSent));
		if (_strings.empty())
			return;

		if ((millis() - _timeSent) > 8000)
		{
			Serial.printf("E940 - Timeout on %s\r\n", _strings.front().c_str());
			SendTopCommand();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Send the command and check set timeouts
	void SendTopCommand()
	{
		if (_strings.empty())
			return;
		Serial.printf( "Sending -> '%s'\r\n",_strings.front().c_str());
		Serial2.println(_strings.front().c_str());
		_timeSent = millis();
	}

private:
	std::vector<std::string> _strings;
	int _timeSent = 0;
	bool _gotReset = false;
	bool _startupComplete = false;
};
