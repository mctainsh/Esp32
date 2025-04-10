#include "esp32-hal.h"
#pragma once

#include <iostream>
#include <sstream>
#include <vector>

#include "MyDisplay.h"
#include "GpsCommandQueue.h"
#include "HandyString.h"
#include "GpsSender.h"
#include "HandyLog.h"

#define MAX_GPS_LOG_LENGTH (200)

class GpsParser
{
private:
	GpsSender _gpsSender;
	MyDisplay &_display;
	GpsCommandQueue _commandQueue;
	bool _gpsConnected = false;			  // Are we receiving GPS data from GPS unit (Does not mean we have location)
	std::vector<std::string> _logHistory; // Last few log messages

public:
	GpsParser(MyDisplay &display) : _display(display),
									_gpsSender(display),
									_commandQueue(display)
	{
		_logHistory.reserve(MAX_GPS_LOG_LENGTH);
	}

	GpsSender &GetGpsSender() { return _gpsSender; }
	GpsCommandQueue &GetCommandQueue() { return _commandQueue; }
	inline std::vector<std::string> GetLogHistory() const { return _logHistory; }

	///////////////////////////////////////////////////////////////////////////
	// Return the number of lines
	bool ReadDataFromSerial(Stream &stream)
	{
		// Read each byte in turn
		int count = 0;
		while (stream.available() > 0)
		{
			char ch = stream.read();

			// Has the packet started?
			// if (_buildBuffer.length() < 1 && ch != '$')
			//{
			//	if (ch != '\r' && ch != '\n')
			//		Serial.printf("Skipping:%c\r\n", ch);
			//	continue;
			//}

			// Skip the CR
			if (ch == '\r')
				continue;

			// The line complete
			if (ch == '\n')
			{
				if (_buildBuffer.length() < 1)
					continue;
				ProcessLine(_buildBuffer);
				_buildBuffer.clear();
				_display.IncrementGpsPackets();
				continue;
			}

			// Is the line too long
			if (_buildBuffer.length() > 254)
			{
				LogGps(StringPrintf("Overflowing %s", _buildBuffer.c_str()));
				_buildBuffer.clear();
				continue;
			}

			// Build the buffer
			_buildBuffer += ch;
		}

		// Check for timeouts
		if ((millis() - _timeOfLastMessage) > 30000)
		{
			_gpsConnected = false;
			_timeOfLastMessage = millis();
			LogGps("W708 - GPS Timeout");
			_commandQueue.SendInitialiseProcess();
		}
		
		// Check output command queue
		_commandQueue.CheckForTimeouts();

		return _gpsConnected;
	}

private:
	unsigned long _timeOfLastMessage = 0; // Millis of last good message
	std::string _buildBuffer;			  // Buffer to make the serial packet up to LF or CR

	///////////////////////////////////////////////////////////////////////////
	// Process a GPS line
	//		$GNGGA,020816.00,2734.21017577,S,15305.98006651,E,4,34,0.6,34.9570,M,41.1718,M,1.0,0*4A
	//		$GNGGA,232306.00,,,,,0,00,9999.0,,,,,,*4E
	//		$devicename,COM1*67										// Reset response (Checksum is wrong)
	//		$command,CONFIG RTK TIMEOUT 10,response: OK*63			// Command response (Note Checksum is wrong)
	void ProcessLine(const std::string &line)
	{
		if (line.length() < 1)
		{
			LogGps("W700 - Too short");
			return;
		}

		_timeOfLastMessage = millis();

		LogGps(StringPrintf("-] '%s'", line.c_str()));

		// Check for command responses
		if (_commandQueue.HasDeviceReset(line))
		{
			_display.ResetGps();
			return;
		}
		if (_commandQueue.IsCommandResponse(line))
			return;

		// Ignore '#' responses
		if (line[0] == '#')
		{
			_commandQueue.CheckForVersion(line);
			return;
		}

		// Verify checksum
		// .. checksum fails for non-$G... responses. This is the case in the following version
		// .. #VERSION,95,GPS,FINE,2333,435235000,0,0,18,906;UM982,R4.10Build11826,HRPT00-S10C-P,...-...,...,2023/11/24*8281e0fb
		if (!VerifyChecksum(line))
			return;

		std::vector<std::string> parts = Split(line, ",");

		// // Skip unused types
		// if (!EndsWith(parts.at(0), "GGA"))
		// {
		// 	Serial.printf("\tNot GGA %s\r\n", line.c_str());
		// 	return;
		// }

		if (parts.size() < 14)
		{
			LogGps(StringPrintf("\tPacket length too short %d", parts.size()));
			return;
		}

		// Skip unused types
		if (parts.at(0) != "$GNGGA")
		{
			LogGps(StringPrintf("\tNot GNGGA %s", line.c_str()));
			return;
		}
		_gpsConnected = true;

		// Read time
		std::string time = parts.at(1);
		if (time.length() > 7)
		{
			time.insert(4, ":");
			time.insert(2, ":");
			time = time.substr(0, 8);
		}
		_display.SetTime(time);

		// Read GPS Quality
		std::string quality = parts.at(6);
		int nQuality = 0;
		if (quality.length() > 0)
		{
			nQuality = std::stoi(quality);
			_display.SetFixMode(nQuality);
		}

		// Location
		double lat = ParseLatLong(parts.at(2), 2, parts.at(3) == "S");
		double lng = ParseLatLong(parts.at(4), 3, parts.at(3) == "E");

		// Height
		double height = 0;
		if (!IsValidDouble(parts.at(9).c_str(), &height))
			height = -1;

		// RTK Age
		std::string rtkAge = parts.at(13);

		_display.SetPosition(lng, lat, height, rtkAge);

		// Serial.printf("\tLL %.10lf, %.10lf\r\n", lat, lng);
		// Serial.print("\t LL ");
		// Serial.print(lat);
		// Serial.print(", ");
		// Serial.println(lat);

		// Satellite count
		std::string satellites = parts.at(7);
		int sat = 0;
		if (satellites.length() > 0)
		{
			sat = std::stoi(satellites);
			_display.SetSatellites(sat);
		}

		_gpsSender.SendHttpData(lat, lng, height, sat, nQuality);
	}

	///////////////////////////////////////////////////////////////////////////
	// Parse the longitude or latitude from
	//		Latitude   = 2734.21017577,S,
	//		Longitude = 15305.98006651,E,
	double ParseLatLong(const std::string &text, int degreeDigits, bool isNegative)
	{
		if (text.length() < degreeDigits)
			return 0.0;
		std::string degree = text.substr(0, degreeDigits);
		std::string minutes = text.substr(degreeDigits);
		double value = std::stod(degree) + (std::stod(minutes) / 60.0);
		return isNegative ? value * -1 : value;
	}

	///////////////////////////////////////////////////////////////////////////////
	// Write to the debug log and keep the last few messages for display
	void LogGps(std::string text)
	{
		// Normal log
		std::string s = StringPrintf("%s %s", Uptime(millis()).c_str(), text.c_str());
#ifdef SERIAL_LOG
		Serial.println(s.c_str());
#endif
		_logHistory.push_back(s);

		TruncateLog(_logHistory);
		//_display.RefreshGpsLog();
	}
};