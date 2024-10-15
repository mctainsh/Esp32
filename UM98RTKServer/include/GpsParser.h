#include "esp32-hal.h"
#pragma once

#include <iostream>
#include <sstream>
#include <vector>

#include "MyDisplay.h"
#include "GpsCommandQueue.h"
#include "HandyString.h"
#include "NTRIPServer.h"

class GpsParser
{
public:
	MyDisplay &_display;
	GpsCommandQueue _commandQueue;
	bool _gpsConnected = false; // Are we receiving GPS data from GPS unit (Does not mean we have location)

	GpsParser(MyDisplay &display) : _display(display)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// Return the number of lines
	bool ReadDataFromSerial(Stream &stream, NTRIPServer &ntripServer)
	{
		int count = 0;

		// Are we sending full binary data to the GPS unit
		if (_commandQueue.StartupComplete())
		{
			// Read the available bytes from stream
			int available = stream.available();
			if (available > 0)
			{
				std::unique_ptr<byte[]> byteArray(new byte[available + 1]);
				stream.readBytes(byteArray.get(), available);

				// Is this an all ASCII packet
				if (IsAllAscii(byteArray.get(), available))
				{
					byteArray[available] = 0;
					Serial.printf("IN %s\r\n", byteArray.get());
				}
				else
				{
					_buildBuffer.clear();

					// Send to RTK Casters
					ntripServer.Loop(byteArray.get(), available);


					_timeOfLastMessage = millis();

					// Write the byte array as hex text to serial
				//	for (int n = 0; n < available; n++)
				//	{
				//		Serial.printf("%02x ", byteArray[n]);
				//	}
				//	Serial.println();
				}
			}
			/*
			Write bb expinfo
			board is restarting
			..........
			$devicename,COM1*67

			77 72 69 74 65 20 62 62 20 65 78 70 69 6e 66 6f 0d 0a
			62 6f 61 72 64 20 69 73 20 72 65 73 74 61 72 74 69 6e 67 0d 0a
			2e 2e 2e 2e 2e 2e 2e 2e 2e 2e 0d 0a
			24 64 65 76 69 63 65 6e 61 6d 65 2c 43 4f 4d 31 2a 36 37 0d 0a

			*/
		}
		else
		{
			// Read the serial data as bytes
			while (stream.available() > 0)
			{
				char ch = stream.read();

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

			// Check output command queue
			_commandQueue.CheckForTimeouts();
		}

		// Check for timeouts
		if ((millis() - _timeOfLastMessage) > 60000)
		{
			_gpsConnected = false;
			_timeOfLastMessage = millis();
			_commandQueue.StartInitialiseProcess();
		}
		return _gpsConnected;
	}

private:
	unsigned long _timeOfLastMessage = 0; // Millis of last good message
	std::string _buildBuffer;			  // Buffer to make the serial packet up to LF or CR

	///////////////////////////////////////////////////////////////////////////
	// Check if the byte array is all ASCII
	bool IsAllAscii(const byte *pBytes, int length) const
	{
		for (int n = 0; n < length; n++)
		{
			if (pBytes[n] == 0x0A || pBytes[n] == 0x0D)
				continue;
			if (pBytes[n] < 32 || pBytes[n] > 126)
				return false;
		}
		return true;
	}

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
			Serial.println("W700 - Too short");
			return;
		}

		_timeOfLastMessage = millis();

		Serial.printf("Processing UN98x %s\r\n", line.c_str());

		// Check for command responses
		if (_commandQueue.HasDeviceReset(line))
		{
			_display.ResetGps();
			return;
		}
		if (_commandQueue.IsCommandResponse(line))
			return;

		// Add data to a build buffer of binary data
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
};