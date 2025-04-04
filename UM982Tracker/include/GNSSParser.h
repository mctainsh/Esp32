#pragma once

#include "HandyString.h"

// Note : Max RTK packet size id 1029 bytes
#define MAX_BUFF 1200

///////////////////////////////////////////////////////////////////////////////
/// Process the GNSS data from TCPIP stream. Format of the packet is
///		Length of body in ASCII hex
///		LF CR
///		Body of packet
///		LF CR
///	Sample packet
///		214 = d0 h
///		64-30-0D-0A-D3-00-CA-45-D0-00-...-00-00-06-90-7C-0D-0A
///		'D' '0' \r \n
///
///		31-62-0D-0A-D3-00-15-3E-E0-00-03-B6-90-C9-D1-D0-89-CF-0B-C1-D4-BA-12-FF-DC-15-00-FA-0C-B9-52-0D-0A
///		'1' 'b' \r \n
///
/// Process
///		1. Wait till we find the first packet that has 0D-0A less than 3 bytes from the start.
///		2. If we find the 0D-0A, then we have the length of the packet.
///
/// DECODING RTCM3
///		See : https://github.com/tomojitakasu/RTKLIB/tree/master
class GNSSParser
{
public:
	///////////////////////////////////////////////////////////////////////
	// Add what we have to the buffer
	int Parse(byte *pBuffer, int bytesRead)
	{
		_completePackets = 0;
		for (int i = 0; i < bytesRead; i++)
			ParseByte(pBuffer[i]);
		return _completePackets;
	}

private:
	const int MaxLength = 1024;
	const int MAX_LENGTH_BYTES = 4;
	std::string _lengthInHex;
	int _packetLength = 0;
	int _bodyLength = 0;
	byte *_body = NULL;
	int _completePackets;
	unsigned char _skippedArray[MAX_BUFF + 2]; // Skipped item array
	int _skippedIndex = 0;					   // Count of skipped items

	const byte RTCM2PREAMB = 0x66; // rtcm ver.2 frame preamble
	const byte RTCM3PREAMB = 0xD3; // rtcm ver.3 frame preamble

	enum PacketBuildState
	{
		GettingFirstLength,
		WaitingFor0A,
		BuildingBody,
		WaitingForEnd0D,
		WaitingForEnd0A
	};

	PacketBuildState _state = PacketBuildState::GettingFirstLength;

	///////////////////////////////////////////////////////////////////////
	// Add a single byte to the build process
	void ParseByte(byte b)
	{
		// Logf("%d '%c, %02x'", _state, b, b);
		switch (_state)
		{
		case PacketBuildState::GettingFirstLength:
			LookingForLength(b);
			break;
		case PacketBuildState::WaitingFor0A:
		{
			if (b == 0x0a)
			{
				_state = PacketBuildState::BuildingBody;
			}
			else
			{
				Logln("E920 - Failed to get OA after length");
				_state = PacketBuildState::GettingFirstLength;
			}
			break;
		}

		case PacketBuildState::BuildingBody:
		{
			// Build the body
			_body[_bodyLength - _packetLength] = b;
			_packetLength--;
			if (_packetLength == 0)
			{
				if (_body[0] != RTCM3PREAMB)
				{
					Logf("E921 - Preamble %02x != RTCM3", _body[0]);
				}
				else
				{
					// TODO : Check packet type and length
					// var length = GetBitU(_body, 14, 10)+3;
					// var type = GetBitU(_body, 24, 12);

					// TODO : Verify Checksum
					// if (RtkCrc24Q(_body, (int)length)!=GetBitU(_body, length*8, 24))
					//{
					//	Console.WriteLine("rtcm3 parity error");
					//}
					// else
					//{
					// Console.WriteLine($"Length {length}");
					//	_port?.Write(_body, 0, _body.Length);
					//}
					_completePackets++;
					LogSkipped();
					Serial2.write(_body, _bodyLength);
				}
				_state = PacketBuildState::WaitingForEnd0D;
			}
			return;
		}
		case PacketBuildState::WaitingForEnd0D:
		{
			if (b == 0x0d)
			{
				_state = PacketBuildState::WaitingForEnd0A;
			}
			else
			{
				Logln("E922 - Failed to get 0D after body");
				_state = PacketBuildState::GettingFirstLength;
			}
			break;
		}
		case PacketBuildState::WaitingForEnd0A:
		{
			if (b == 0x0a)
			{
				// TODO : Process the body
				// Console.WriteLine(BitConverter.ToString(_body));
			}
			else
			{
				Logln("E925 - Failed to get 0A after body");
			}
			_state = PacketBuildState::GettingFirstLength;
			break;
		}
		default:
		{
			Logln("E930 - Unknown state " + _state);
			break;
		}
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Add to buffer of skipped data
	void AddToSkipped(char ch)
	{
		if (_skippedIndex >= MAX_BUFF)
		{
			Logln("Skip buffer overflowed");
			_skippedIndex = 0;
		}
		_skippedArray[_skippedIndex++] = ch;
	}

	///////////////////////////////////////////////////////////////////////
	// Called only when length has not been received yet
	void LookingForLength(byte b)
	{
		// Find the length before 0d
		if (b == 0x0d)
		{
			// Convert lengthInHex to an integer
			if (IsValidHex(_lengthInHex))
			{
				int temp = std::stoi(_lengthInHex, nullptr, 16);
				std::string check = StringPrintf("%x", temp);

				// Check if the length is too short
				if (temp < 1)
				{
					Logf("E905 - Length '%s' is too small %d", _lengthInHex.c_str(), temp);
				}
				// Check if the length is too long
				else if (temp > MaxLength)
				{
					Logf("E907 - Length '%s' too long %d", _lengthInHex.c_str(), temp);
				}
				// Check the read length matches the calculated length
				else if (strcasecmp(_lengthInHex.c_str(), check.c_str()) == 0)
				{
					_packetLength = temp;
					_bodyLength = temp;
					_state = PacketBuildState::WaitingFor0A;
					if (_body != NULL)
						delete[] _body;
					_body = new byte[_bodyLength];
				}
				else
				{
					Logf("E900 - Failed to parse length '%s' as %s", _lengthInHex.c_str(), check.c_str());
				}
			}
			_lengthInHex = "";
			return;
		}

		// Skip non hex characters upper and lower case
		// const string GOOD_HEX = "0123456789ABCDEF";
		if (('0' > b || b > '9') && ('A' > b || b > 'F') && ('a' > b || b > 'f'))
		{
			AddToSkipped(b);
			// Logf("E910 - Non Hex number '%s'", _lengthInHex.c_str());
			_lengthInHex = "";
			return;
		}

		// Is it too long?
		if (_lengthInHex.length() > MAX_LENGTH_BYTES)
		{
			Logf("E900 - '%s' max length%d", _lengthInHex.c_str(), _lengthInHex.length());
			_lengthInHex = "";
			return;
		}

		// Add to the length
		_lengthInHex += static_cast<char>(b);
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

	///////////////////////////////////////////////////////////////////////
	// Log the skipped data
	void LogSkipped()
	{
		if (_skippedIndex < 1)
			return;
		if (_skippedIndex == 1 && _skippedArray[0] == 0x0A)
		{
			// Don't dump the end of ASCII line
		}
		else
		{
			_skippedArray[_skippedIndex] = 0;
			if (IsAllAscii(_skippedArray, _skippedIndex))
				Logf("Skipped [%d] %s", _skippedIndex, _skippedArray);
			else
				Logf("Skipped [%d] %s", _skippedIndex, HexDump(_skippedArray, _skippedIndex).c_str());
			//_missedBytesDuringError += _skippedIndex;
		}
		_skippedIndex = 0;
	}
};