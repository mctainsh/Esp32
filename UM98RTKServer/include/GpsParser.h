#pragma once

#include <iostream>
#include <sstream>
#include <vector>

#include "MyDisplay.h"
#include "GpsCommandQueue.h"
#include "HandyString.h"
#include "NTRIPServer.h"
#include "Global.h"

// Note : Max RTK packet size id 1029 bytes
#define MAX_BUFF 1200

static const unsigned int tbl_CRC24Q[] = {
	0x000000, 0x864CFB, 0x8AD50D, 0x0C99F6, 0x93E6E1, 0x15AA1A, 0x1933EC, 0x9F7F17,
	0xA18139, 0x27CDC2, 0x2B5434, 0xAD18CF, 0x3267D8, 0xB42B23, 0xB8B2D5, 0x3EFE2E,
	0xC54E89, 0x430272, 0x4F9B84, 0xC9D77F, 0x56A868, 0xD0E493, 0xDC7D65, 0x5A319E,
	0x64CFB0, 0xE2834B, 0xEE1ABD, 0x685646, 0xF72951, 0x7165AA, 0x7DFC5C, 0xFBB0A7,
	0x0CD1E9, 0x8A9D12, 0x8604E4, 0x00481F, 0x9F3708, 0x197BF3, 0x15E205, 0x93AEFE,
	0xAD50D0, 0x2B1C2B, 0x2785DD, 0xA1C926, 0x3EB631, 0xB8FACA, 0xB4633C, 0x322FC7,
	0xC99F60, 0x4FD39B, 0x434A6D, 0xC50696, 0x5A7981, 0xDC357A, 0xD0AC8C, 0x56E077,
	0x681E59, 0xEE52A2, 0xE2CB54, 0x6487AF, 0xFBF8B8, 0x7DB443, 0x712DB5, 0xF7614E,
	0x19A3D2, 0x9FEF29, 0x9376DF, 0x153A24, 0x8A4533, 0x0C09C8, 0x00903E, 0x86DCC5,
	0xB822EB, 0x3E6E10, 0x32F7E6, 0xB4BB1D, 0x2BC40A, 0xAD88F1, 0xA11107, 0x275DFC,
	0xDCED5B, 0x5AA1A0, 0x563856, 0xD074AD, 0x4F0BBA, 0xC94741, 0xC5DEB7, 0x43924C,
	0x7D6C62, 0xFB2099, 0xF7B96F, 0x71F594, 0xEE8A83, 0x68C678, 0x645F8E, 0xE21375,
	0x15723B, 0x933EC0, 0x9FA736, 0x19EBCD, 0x8694DA, 0x00D821, 0x0C41D7, 0x8A0D2C,
	0xB4F302, 0x32BFF9, 0x3E260F, 0xB86AF4, 0x2715E3, 0xA15918, 0xADC0EE, 0x2B8C15,
	0xD03CB2, 0x567049, 0x5AE9BF, 0xDCA544, 0x43DA53, 0xC596A8, 0xC90F5E, 0x4F43A5,
	0x71BD8B, 0xF7F170, 0xFB6886, 0x7D247D, 0xE25B6A, 0x641791, 0x688E67, 0xEEC29C,
	0x3347A4, 0xB50B5F, 0xB992A9, 0x3FDE52, 0xA0A145, 0x26EDBE, 0x2A7448, 0xAC38B3,
	0x92C69D, 0x148A66, 0x181390, 0x9E5F6B, 0x01207C, 0x876C87, 0x8BF571, 0x0DB98A,
	0xF6092D, 0x7045D6, 0x7CDC20, 0xFA90DB, 0x65EFCC, 0xE3A337, 0xEF3AC1, 0x69763A,
	0x578814, 0xD1C4EF, 0xDD5D19, 0x5B11E2, 0xC46EF5, 0x42220E, 0x4EBBF8, 0xC8F703,
	0x3F964D, 0xB9DAB6, 0xB54340, 0x330FBB, 0xAC70AC, 0x2A3C57, 0x26A5A1, 0xA0E95A,
	0x9E1774, 0x185B8F, 0x14C279, 0x928E82, 0x0DF195, 0x8BBD6E, 0x872498, 0x016863,
	0xFAD8C4, 0x7C943F, 0x700DC9, 0xF64132, 0x693E25, 0xEF72DE, 0xE3EB28, 0x65A7D3,
	0x5B59FD, 0xDD1506, 0xD18CF0, 0x57C00B, 0xC8BF1C, 0x4EF3E7, 0x426A11, 0xC426EA,
	0x2AE476, 0xACA88D, 0xA0317B, 0x267D80, 0xB90297, 0x3F4E6C, 0x33D79A, 0xB59B61,
	0x8B654F, 0x0D29B4, 0x01B042, 0x87FCB9, 0x1883AE, 0x9ECF55, 0x9256A3, 0x141A58,
	0xEFAAFF, 0x69E604, 0x657FF2, 0xE33309, 0x7C4C1E, 0xFA00E5, 0xF69913, 0x70D5E8,
	0x4E2BC6, 0xC8673D, 0xC4FECB, 0x42B230, 0xDDCD27, 0x5B81DC, 0x57182A, 0xD154D1,
	0x26359F, 0xA07964, 0xACE092, 0x2AAC69, 0xB5D37E, 0x339F85, 0x3F0673, 0xB94A88,
	0x87B4A6, 0x01F85D, 0x0D61AB, 0x8B2D50, 0x145247, 0x921EBC, 0x9E874A, 0x18CBB1,
	0xE37B16, 0x6537ED, 0x69AE1B, 0xEFE2E0, 0x709DF7, 0xF6D10C, 0xFA48FA, 0x7C0401,
	0x42FA2F, 0xC4B6D4, 0xC82F22, 0x4E63D9, 0xD11CCE, 0x575035, 0x5BC9C3, 0xDD8538};

class GpsParser
{
	// The state of the build
	enum BuildState
	{
		BuildStateNone,
		BuildStateBinaryPre1,
		BuildStateBinaryPre2,
		BuildStateBinary,
		BuildStateAscii
	};

private:
	unsigned long _timeOfLastMessage = 0;	 // Millis of last good message
	std::string _buildBuffer;				 // Buffer to make the serial packet up to LF or CR
	unsigned char _byteArray[MAX_BUFF + 1];	 // Buffer to hold the binary data
	int _binaryIndex = 0;					 // Index of the binary data
	int _binaryLength = 0;					 // Length of the binary packet
	std::vector<std::string> _logHistory;	 // Last few log messages
	BuildState _buildState = BuildStateNone; // Where we are with the build of a packet
public:
	MyDisplay &_display;
	GpsCommandQueue _commandQueue;
	bool _gpsConnected = false; // Are we receiving GPS data from GPS unit (Does not mean we have location)

	GpsParser(MyDisplay &display) : _display(display)
	{
		_logHistory.reserve(MAX_LOG_LENGTH);
	}

	inline std::vector<std::string> GetLogHistory() const { return _logHistory; }
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

				auto bytes = byteArray.get();

				// Send to RTK Casters
				ntripServer0.Loop(bytes, available);
				ntripServer1.Loop(bytes, available);
				ntripServer2.Loop(bytes, available);

				// Process the binary data (Npot important for data but handy for stats)
				//for (int n = 0; n < available; n++)
				//	ProcessGpsSerialByte(bytes[n]);

				_timeOfLastMessage = millis();
			}
		}
		else
		{
			ProcessStreamBeforeConnected(stream);

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
	// Read the latest GPS data and check for timeouts
	void ProcessStreamBeforeConnected(Stream &stream)
	{
		char ch = '\0';
		while (stream.available() > 0)
		{
			ch = stream.read();
			ProcessGpsSerialByte(ch);
		}
	}


	///////////////////////////////////////////////////////////////////////////
	// Process a new character from the GPS unit
	void ProcessGpsSerialByte(char ch)
	{
		switch (_buildState)
		{
		// Check for start character
		case BuildStateNone:
			switch (ch)
			{
			case '$':
			case '#':
				LogX("Build ASCII");
				_buildBuffer.clear();
				_buildBuffer += ch;
				_buildState = BuildStateAscii;
				break;
			case 0x0a:
				break;
			case 0xD3:
				LogX("Build BINARY");
				_buildState = BuildStateBinary;
				_binaryLength = 0;
				//_binaryIndex = 0;
				_binaryIndex = 1;
				_byteArray[0] = ch;
				break;
			default:
				LogX(StringPrintf("Skip '%c', 0x%02x", ch, ch));
				break;
			}
			break;

		// Work the binary buffer
		case BuildStateBinary:
			BuildBinary(ch);
			break;

		// Plain text processing
		case BuildStateAscii:
			BuildAscii(ch);
			break;
		default:
			LogX(StringPrintf("Unknown state %d", _buildState));
			_buildState = BuildStateNone;
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Process a new byte in binary format
	void BuildBinary(char ch)
	{
		_byteArray[_binaryIndex++] = ch;

		if (_binaryIndex < 4)
			return;

		// Extract length
		if (_binaryLength == 0 && _binaryIndex >= 4)
		{
			auto lengthPrefix = GetUInt(8, 14 - 8);
			if (lengthPrefix != 0)
			{
				LogX(StringPrintf("Binary length prefix too big %02xh %02xh", _byteArray[1], _byteArray[2]));
				DumpBinaryBufferFalseStart();
				return;
			}
			_binaryLength = GetUInt(14, 10) + 6;
			if (_binaryLength >= MAX_BUFF)
			{
				LogX(StringPrintf("Binary length too big %d", _binaryLength));
				DumpBinaryBufferFalseStart();
				return;
			}
			LogX(StringPrintf("Buffer length %d", _binaryLength));
			return;
		}
		if (_binaryIndex >= MAX_BUFF)
		{
			// Dump as HEX
			LogX(StringPrintf("Buffer overflow %d", _binaryIndex));
			DumpBinaryBufferFalseStart();
			return;
		}

		// Have we got the full buffer
		if (_binaryIndex >= _binaryLength)
		{
			// Verify checksum
			auto parity = GetUInt((_binaryLength - 3) * 8, 24);
			auto calculated = RtkCrc24();
			if (parity != calculated)
			{
				LogX(StringPrintf("Checksum failed %d != %d", parity, calculated));
				DumpBinaryBufferFalseStart();
				return;
			}

			auto type = GetUInt(24, 12);
			LogX(StringPrintf("Type %d -> %s", type, HexDump(_byteArray, _binaryIndex).c_str()));
			_buildState = BuildStateNone;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Build the ASCII buffer
	void BuildAscii(char ch)
	{
		// The line complete
		if (ch == '\r' || ch == '\n')
		{
			if (_buildBuffer.length() < 1)
				return;
			ProcessLine(_buildBuffer);
			_buildBuffer.clear();
			_buildState = BuildStateNone;
			return;
		}

		// Is the line too long
		if (_buildBuffer.length() > 254)
		{
			LogX(StringPrintf("Overflowing %s\r\n", _buildBuffer.c_str()));
			_buildBuffer.clear();
			_buildState = BuildStateNone;
			return;
		}

		// Check for non ascii characters
		if (ch < 32 || ch > 126)
		{
			LogX(StringPrintf("Non-ASCII 0x%02x '%c' in %s", ch, ch, _buildBuffer.c_str()));
			_buildBuffer.clear();
			_buildState = BuildStateNone;
			return;
		}

		// Append to the build the buffer
		_buildBuffer += ch;
	}

	///////////////////////////////////////////////////////////////////////////
	// Binary buffer has a false start so dump the bad data and test for a new start
	// WARNING : This is a recursive function and could cause a stack overflow
	void DumpBinaryBufferFalseStart()
	{
		LogX(StringPrintf("Dump IN '%s'", HexDump(_byteArray, _binaryIndex).c_str()));
		_buildState = BuildStateNone;

		// Find next D3 in the buffer
		for (int n = 1; n < _binaryIndex; n++)
		{
			if (_byteArray[n] != 0xD3)
			{
				LogX(StringPrintf("Dump '%c', 0x%02x", _byteArray[n], _byteArray[n]));
				continue;
			}

			// Copy the buffer to the start
			int newLength = _binaryIndex - n;
			memmove(_byteArray, _byteArray + n, newLength);
			_binaryIndex = newLength;
			_binaryLength = 0;
			LogX(StringPrintf("Dump OUT '%s'", HexDump(_byteArray, _binaryIndex).c_str()));
			_buildState = BuildStateBinary;
			return;
		}
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

		// If line contains any non-simple ascii characters, show in hex
		LogX(StringPrintf("GPS [- '%s'", line.c_str()));

		bool isSimpleAscii = true;
		for (int n = 0; n < line.length(); n++)
		{
			if (line[n] < 32 || line[n] > 126)
			{
				isSimpleAscii = false;
				break;
			}
		}
		if (!isSimpleAscii)
		{
			std::string hex = "";
			for (int n = 0; n < line.length(); n++)
			{
				hex += StringPrintf("%02x ", line[n]);
			}
			LogX(StringPrintf("GPS [x] %s", hex.c_str()));
		}

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
		auto s = Logln(text.c_str());
		_logHistory.push_back(s);
		if (_logHistory.size() > MAX_LOG_LENGTH)
			_logHistory.erase(_logHistory.begin());
		_display.RefreshGpsLog();
	}

private:
	////////////////////////////////////////////////////////////////////////////
	// Pull an unsigned integer from a byte array
	// @param buff The byte array
	// @param pos The position in the byte array
	// @param len Number of bits to read
	//		1 = byte
	//		2 = word
	//		4 = long
	unsigned int GetUInt(int pos, int len)
	{
		unsigned int bits = 0;
		for (int i = pos; i < pos + len; i++)
			bits = (bits << 1) + ((_byteArray[i / 8] >> (7 - i % 8)) & 1u);
		return bits;
	}

	////////////////////////////////////////////////////////////////////////////
	// Calculate the CRC24Q checksum. Note, the last 3 bytes are the checksum
	// so we do not include them in the calculation
	// RTCM3 format is
	//  +----------+--------+-----------+--------------------+----------+
	//  | Preamble | 000000 |  length   |    data message    |  parity  |
	//  +----------+--------+-----------+--------------------+----------+
	//  |<--- 8 --->|<--6-->|<-- 10 --->|<--- length x 8 --->|<-- 24 -->|
	// @return The checksum
	unsigned int RtkCrc24()
	{
		unsigned int crc = 0;
		for (int i = 0; i < _binaryLength - 3; i++)
			crc = ((crc << 8) & 0xFFFFFF) ^ tbl_CRC24Q[(crc >> 16) ^ _byteArray[i]];
		return crc;
	}
};