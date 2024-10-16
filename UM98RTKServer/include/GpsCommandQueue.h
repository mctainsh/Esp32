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

	inline bool StartupComplete() const { return _startupComplete; }

	inline bool GotReset() const { return _gotReset; }

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
		if (_gotReset && _strings.empty())
		{
			Logf("GPS Startup Commands Complete\r\n");
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
		Logf("GPS Queue StartInitialiseProcess %d\r\n", _gotReset);
		_strings.clear();
		if (_gotReset)
		{
			// Setup RTCM V3
			_strings.push_back("version");				  // Used to determine device type
			_strings.push_back("CONFIG SIGNALGROUP 3 6"); // (for UM982)
			_strings.push_back("MODE BASE TIME 60 5");	  // Set base mode with 60 second startup and 5m optimized save error
			_strings.push_back("rtcm1005 30");			  // Base station antenna reference point (ARP) coordinates
			_strings.push_back("rtcm1033 30");			  // Receiver and antenna description
			_strings.push_back("rtcm1077 1");			  // GPS MSM7. The type 7 Multiple Signal Message format for the USA’s GPS system, popular.
			_strings.push_back("rtcm1087 1");			  // GLONASS MSM7. The type 7 Multiple Signal Message format for the Russian GLONASS system.
			_strings.push_back("rtcm1097 1");			  // Galileo MSM7. The type 7 Multiple Signal Message format for Europe’s Galileo system.
			_strings.push_back("rtcm1117 1");			  // QZSS MSM7. The type 7 Multiple Signal Message format for Japan’s QZSS system.
			_strings.push_back("rtcm1127 1");			  // BeiDou MSM7. The type 7 Multiple Signal Message format for China’s BeiDou system.
			_strings.push_back("saveconfig");
		}
		else
		{
			_strings.push_back("freset");
		}

		SendTopCommand();
	}

	///////////////////////////////////////////////////////////////////////////
	// Check for version response
	// .. version response looks like "#VERSION,0,GPS,UNKNOWN,0,0,0,0,0,1261;UM982,R4.10Build11826,HRPT00-S10C-P,2310415000012-LR23A0225104240,ff27289609cf869d,2023/11/24*4d0ec3ba"

	//////////////////////////////////////////////////////////////////////////
	// Check queue for timeouts
	void CheckForTimeouts()
	{
		// Logf("Check TO %d,  %d\r\n", _strings.size(), (millis() - _timeSent));
		if (_strings.empty())
			return;

		if ((millis() - _timeSent) > 8000)
		{
			Logf("E940 - Timeout on %s\r\n", _strings.front().c_str());
			SendTopCommand();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Send the command and check set timeouts
	void SendTopCommand()
	{
		if (_strings.empty())
			return;
		Logf("Sending -> '%s'\r\n", _strings.front().c_str());
		Serial2.println(_strings.front().c_str());
		_timeSent = millis();
	}

private:
	std::vector<std::string> _strings;
	int _timeSent = 0;
	bool _gotReset = false;
	bool _startupComplete = false;
};
