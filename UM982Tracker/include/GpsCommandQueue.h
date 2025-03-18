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
	int _timeSent = 0;						 // Time of last message send. Used for timeouts
	bool _sendSetupAfterReset = false;		 // Setup the send all setting after the next reset
	int _resetCount = 0;					 // Total resets
	std::string _deviceType;				 // Device type like UM982
	std::string _deviceFirmware = "UNKNOWN"; // Firmware version
	std::string _deviceSerial = "UNKNOWN";	 // Serial number
	MyDisplay &_display;

public:
	///////////////////////////////////////////////////////////////////////////
	// Constructor
	GpsCommandQueue(MyDisplay &display) : _display(display)
	{
		_strings.push_back("version");
	}

	const int GetResetCount() const { return _resetCount; }
	inline const std::string &GetDeviceType() const { return _deviceType; }
	inline const std::string &GetDeviceFirmware() const { return _deviceFirmware; }
	inline const std::string &GetDeviceSerial() const { return _deviceSerial; }

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

		Logf("I200 - Reset received");
		_resetCount++;
		_display.SetRtkStatus("GPS Reset");
		if (_sendSetupAfterReset)
			SendInitialiseProcess();
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// Start the GPS chip reset process
	void StartResetProcess()
	{
		_sendSetupAfterReset = true;
		_strings.push_back("freset");
		SendTopCommand();
	}

	///////////////////////////////////////////////////////////////////////////
	// Check if the GPS receiver has reset itself and send all the commands
	void SendInitialiseProcess()
	{
		// Load the commands
		_strings.clear();

		Logln("GPS Queue StartInitialiseProcess");
		//_strings.push_back("config signalgroup 3 6");
		_strings.push_back("MODE ROVER");
		_strings.push_back("CONFIG RTK TIMEOUT 10");
		_strings.push_back("GNGGA 1");
		_strings.push_back("saveconfig");
		_sendSetupAfterReset = false;

		SendTopCommand();
	}

	///////////////////////////////////////////////////////////////////////////
	/// @brief Process a line of text from the GPS unit checking got version information
	///	#VERSION,0,GPS,UNKNOWN,0,0,0,0,0,1261;UM982,R4.10Build11826,HRPT00-S10C-P,2310415000012-LR23A0225104240,ff27289609cf869d,2023/11/24*4d0ec3ba
	//  #VERSION,0,GPS,UNKNOWN,0,0,0,0,0,1271;UM982,R4.10Build11826,HRPT00-S10C-P,2310415000012-LR23A0225104240,ff27289609cf869d,2023/11/24*92084e78'
	inline void CheckForVersion(const std::string &str)
	{
		if (!StartsWith(str, "#VERSION"))
			return;

		auto sections = Split(str, ";");
		if (sections.size() < 1)
		{
			Logf("DANGER 301 : Unknown sections '%s' Detected", str.c_str());
			return;
		}

		auto parts = Split(sections[1], ",");
		if (parts.size() < 5)
		{
			Logf("DANGER 302 : Unknown split '%s' Detected", str.c_str());
			return;
		}
		_deviceType = parts[0];
		_deviceFirmware = parts[1];
		auto serialPart = Split(parts[3], "-");
		_deviceSerial = serialPart[0];
		_display.RefreshScreen();

		// New documentation for Unicore. The new firmware (Build17548) has 50 Hz and QZSS L6 reception instead of Galileo E6.
		// .. From now on, we install the Build17548 firmware on all new UM980 receivers. So we have a new advantage - you can
		// .. enable L6 instead of E6 by changing SIGNALGOUP from 2 to 10. Another thing is that this is only needed in Japan
		// .. and countries close to it.

		// Setup signal groups
		std::string command = "CONFIG SIGNALGROUP 3 6"; // Assume for UM982)
		if (_deviceType == "UM982")
		{
			Logln("UM982 Detected");
		}
		else if (_deviceType == "UM980")
		{
			Logln("UM980 Detected");
			command = "CONFIG SIGNALGROUP 2"; // (for UM980)
		}
		else
		{
			Logf("DANGER 303 Unknown Device '%s' Detected in %s", _deviceType.c_str(), str.c_str());
		}

		// NOTE : Setting Signal group resets the device the first whenever it changes type 
		_strings.push_back(command);
		if( _strings.size() == 1)
			SendTopCommand();
	}

	//////////////////////////////////////////////////////////////////////////
	// Check queue for timeouts
	void CheckForTimeouts()
	{
		if (_strings.empty())
			return;

		if ((millis() - _timeSent) > 8000)
		{
			Logf("E940 - Timeout on %s", _strings.front().c_str());
			SendTopCommand();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Send the command and check set timeouts
	void SendTopCommand()
	{
		if (_strings.empty())
			return;

		Logf("Sending command -> '%s'", _strings.front().c_str());
		Serial2.println(_strings.front().c_str());
		_timeSent = millis();
	}
};
