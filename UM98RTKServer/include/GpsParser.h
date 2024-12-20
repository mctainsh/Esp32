#pragma once

#include <iostream>
#include <sstream>
#include <vector>

#include "MyDisplay.h"
#include "GpsCommandQueue.h"
#include "HandyString.h"
#include "NTRIPServer.h"
#include "Global.h"

class GpsParser
{
private:
	unsigned long _timeOfLastMessage = 0;	 // Millis of last good message
	std::string _buildBuffer;				 // Buffer to make the serial packet up to LF or CR
	std::vector<std::string> _logHistory;	 // Last few log messages
public:
	MyDisplay &_display;
	GpsCommandQueue _commandQueue;
	bool _gpsConnected = false; // Are we receiving GPS data from GPS unit (Does not mean we have location)

	GpsParser(MyDisplay &display) : _display(display)
	{
		_logHistory.reserve(MAX_LOG_LENGTH);
	}

	inline const std::vector<std::string> &GetLogHistory() const { return _logHistory; }
	inline const GpsCommandQueue &GetCommandQueue() const { return _commandQueue; }

	///////////////////////////////////////////////////////////////////////////
	// Read the latest GPS data and check for timeouts
	bool ReadDataFromSerial(Stream &stream, NTRIPServer &ntripServer0, NTRIPServer &ntripServer1, NTRIPServer &ntripServer2)
	{
		int count = 0;

		// Are we sending full binary data to the GPS unit
		if (_commandQueue.StartupComplete())
		{
			// Read the available bytes from stream
			int available = stream.available();
			if (available > 0)
			{
				_display.IncrementGpsPackets();
				// TODO Just use a single buffer over and over
				std::unique_ptr<byte[]> byteArray(new byte[available + 1]);
				stream.readBytes(byteArray.get(), available);

				// Is this an all ASCII packet
				if (available > 78 && byteArray[available - 2] == 0x0D && byteArray[available - 1] == 0x0A && IsAllAscii(byteArray.get(), available))
				{
					byteArray[available] = 0;
					const char *pStr = (const char *)byteArray.get();
					// Does it end in a CR or LF
					if (StartsWith(pStr, "Write bb expinfo") ||
						StartsWith(pStr, "board is restarting") ||
						StartsWith(pStr, "..........") ||
						StartsWith(pStr, "$devicename,COM"))
					{
						LogX(StringPrintf("SKIPPING Special command %s", pStr));
						return _gpsConnected;
					}
				}

				// Process the binary data
				_buildBuffer.clear();
				_gpsConnected = true;

				// Send to RTK Casters
				ntripServer0.Loop(byteArray.get(), available);
				ntripServer1.Loop(byteArray.get(), available);
				ntripServer2.Loop(byteArray.get(), available);

				_timeOfLastMessage = millis();

				// Write the byte array as hex text to serial
				//	for (int n = 0; n < available; n++)
				//	{
				//		Logf("%02x ", byteArray[n]);
				//	}
				//	Logln();
			}
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
					continue;
				}

				// Is the line too long
				if (_buildBuffer.length() > 254)
				{
					LogX(StringPrintf("Overflowing %s\r\n", _buildBuffer.c_str()));
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
		if (_commandQueue.GotReset() && (millis() - _timeOfLastMessage) > 60000)
		{
			_gpsConnected = false;
			_timeOfLastMessage = millis();
			_commandQueue.StartInitialiseProcess();
			_display.UpdateGpsStarts(false, true);
		}
		return _gpsConnected;
	}

	///////////////////////////////////////////////////////////////////////////
	// Check if the byte array is all ASCII
	static bool IsAllAscii(const byte *pBytes, int length)
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
			LogX("W700 - Too short");
			return;
		}

		_timeOfLastMessage = millis();

		LogX(StringPrintf("GPS [- '%s'", line.c_str()));

		// Check for command responses
		if (_commandQueue.HasDeviceReset(line))
		{
			_display.UpdateGpsStarts(true, false);
			return;
		}
		
		if (_commandQueue.IsCommandResponse(line))
			return;

		_commandQueue.CheckForVersion(line);
	}

	///////////////////////////////////////////////////////////////////////////////
	// Write to the debug log and keep the last few messages for display
	void LogX(std::string text)
	{
		Logln(text.c_str());
		_logHistory.push_back(StringPrintf("%d %s", millis(), text.c_str()));
		if (_logHistory.size() > MAX_LOG_LENGTH)
			_logHistory.erase(_logHistory.begin());
		_display.RefreshGpsLog();
	}
};