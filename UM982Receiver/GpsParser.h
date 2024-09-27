#pragma once

#include <iostream>
#include <sstream>
#include <vector>

#include "MyDisplay.h"
#include "GpsCommandQueue.h"

class GpsParser
{
  public:
	MyDisplay& _display;
	GpsCommandQueue _commandQueue;

	GpsParser(MyDisplay& display)
	  : _display(display)
	{
	}


	///////////////////////////////////////////////////////////////////////////
	// Return the number of lines
	void ReadDataFromSerial(Stream& stream)
	{
		int count = 0;
		while (stream.available() > 0)
		{
			char ch = stream.read();

			// Has the packet started?
			//if (_buildBuffer.length() < 1 && ch != '$')
			//{
			//	if (ch != '\r' && ch != '\n')
			//		Serial.printf("Skipping:%c\r\n", ch);
			//	continue;
			//}

			// The line complete
			if (ch == '\r' || ch == '\n')
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
				Serial.printf("Overflowing %s\r\n", _buildBuffer.c_str());
				_buildBuffer.clear();
				continue;
			}

			// Build the buffer
			_buildBuffer += ch;
		}
	}
  private:

	bool _firstSendGGA = true;
	///////////////////////////////////////////////////////////////////////////
	// Process a GPS line
	//		$GNGGA,020816.00,2734.21017577,S,15305.98006651,E,4,34,0.6,34.9570,M,41.1718,M,1.0,0*4A
	//		$GNGGA,232306.00,,,,,0,00,9999.0,,,,,,*4E
	//		$devicename,COM1*67										// Reset response (Checksum is wrong)
	//		$command,CONFIG RTK TIMEOUT 10,response: OK*63			// Command response (Note Checksum is wrong)
	void ProcessLine(const std::string& line)
	{
		if (line.length() < 1)
		{
			Serial.println("W700 - Too short");
			return;
		}

		Serial.printf("Processing UN98x %s\r\n", line.c_str());

		// Check for command responses
		if (_commandQueue.HasDeviceReset(line))
			return;
		if (_commandQueue.IsCommandResponse(line))
			return;

		// Ignore '#' responses
		if (line[0] == '#')
		{
			Serial.println("\tW701 - Ignored");
			return;
		}

		// Verify checksum
		// .. checksum fails for non-$G... recponses. This is the case in the following version
		// .. #VERSION,95,GPS,FINE,2333,435235000,0,0,18,906;UM982,R4.10Build11826,HRPT00-S10C-P,...-...,...,2023/11/24*8281e0fb
		if (!VerifyChecksum(line))
			return;

		std::vector<std::string> parts = Split(line, ",");

		// Ask gor GGA if reset
		if (line == "$devicename,COM1*67" && _firstSendGGA)
		{
			Serial2.println("GNGGA 1");
			_firstSendGGA = false;
			return;
		}

		// Skip unused types
		if (!EndsWith(parts.at(0), "GGA"))
		{
			Serial.printf("\tNot GGA %s\r\n", line.c_str());
			return;
		}

		if (parts.size() < 14)
		{
			Serial.printf("\tPacket length too short %d\r\n", parts.size());
			return;
		}

		// Skip unused tyles
		if (parts.at(0) != "$GNGGA")
		{
			Serial.printf("\tNot GGA %s\r\n", line.c_str());
			return;
		}

		// Read time
		std::string time = parts.at(1);
		if (time.length() > 7)
		{
			time.insert(4, ":");
			time.insert(2, ":");
			time = time.substr(0, 8);
		}
		_display.SetTime(time);

		// Gead GPS Quality
		std::string quality = parts.at(6);
		if (quality.length() > 0)
			_display.SetFixMode(std::stoi(quality));

		double lat = ParseLatLong(parts.at(2), 2, parts.at(3) == "S");
		double lng = ParseLatLong(parts.at(4), 3, parts.at(3) == "E");
		_display.SetPosition(lng, lat);
		//Serial.printf("\tLL %.10lf, %.10lf\r\n", lat, lng);
		//Serial.print("\t LL ");
		//Serial.print(lat);
		//Serial.print(", ");
		//Serial.println(lat);

		// Satellite count
		std::string satellites = parts.at(7);
		if (satellites.length() > 0)
			_display.SetSatellites(std::stoi(satellites));
	}


	///////////////////////////////////////////////////////////////////////////
	// Parse the longitude or latitlue from
	//		Latitude   = 2734.21017577,S,
	//		Longituide = 15305.98006651,E,
	double ParseLatLong(const std::string& text, int degreeDigits, bool isNegative)
	{
		if (text.length() < degreeDigits)
			return 0.0;
		std::string degree = text.substr(0, degreeDigits);
		std::string minutes = text.substr(degreeDigits);
		double value = std::stod(degree) + (std::stod(minutes) / 60.0);
		return isNegative ? value * -1 : value;
	}


	std::string _buildBuffer;
	/*const std::string DUMMY_DATA[] = {
		"$GNGGA,020813.00,2734.21017606,S",
		",15305.98006887,E,4,33,0.6,34.94",
		"32,M,41.1718,M,1.0,0*4F\r\n",
		"$GNGGA,020814.00,2734.21017450,S",
		", 15305.98006438, E, 4, 34, 0.6, 34.97",
		"13,M,41.1718,M,1.0,0*46\r\n$GNGGA,020815.00,2734.21017375,S",
		",15305.98006392,E,4,34,0.6,34.9470,M,41.1718,M,1.0,0*46\r\n$GNGGA,020816.00,2734.21017577,S,15305.98006651,E,4,34,0.6,34.9570,M,41.1718,M,1.0,0*4A\r\n",
		"$GNGGA,020817.00,2734.21017373,S,15305.98006743,E,4,34,0.6,34.9398,M,41.1718,M,1.0,0*4B\r\n"
	};*/
};