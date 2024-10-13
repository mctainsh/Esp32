#pragma once

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
	int Parse(byte* pBuffer, int bytesRead)
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
	byte* _body = NULL;
	int _completePackets;

	const byte RTCM2PREAMB = 0x66;	// rtcm ver.2 frame preamble
	const byte RTCM3PREAMB = 0xD3;	// rtcm ver.3 frame preamble

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
		//Serial.printf("%d '%c, %02x'", _state, b, b);
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
						Serial.println("E920 - Failed to get OA after length");
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
							Serial.printf("E921 - Preamble %02x != RTCM3\r\n", _body[0]);
						}
						else
						{
							// TODO : Check packet type and length
							//var length = GetBitU(_body, 14, 10)+3;
							//var type = GetBitU(_body, 24, 12);

							// TODO : Verify Checksum
							//if (RtkCrc24Q(_body, (int)length)!=GetBitU(_body, length*8, 24))
							//{
							//	Console.WriteLine("rtcm3 parity error");
							//}
							//else
							//{
							//Console.WriteLine($"Length {length}");
							//	_port?.Write(_body, 0, _body.Length);
							//}
							_completePackets++;
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
						Serial.println("E922 - Failed to get 0D after body");
						_state = PacketBuildState::GettingFirstLength;
					}
					break;
				}
			case PacketBuildState::WaitingForEnd0A:
				{
					if (b == 0x0a)
					{
						// TODO : Process the body
						//Console.WriteLine(BitConverter.ToString(_body));
					}
					else
					{
						Serial.println("E925 - Failed to get 0A after body");
					}
					_state = PacketBuildState::GettingFirstLength;
					break;
				}
			default:
				{
					Serial.println("E930 - Unknown state " + _state);
					break;
				}
		}
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

				// Check if the length is too long
				if (temp > MaxLength)
				{
					Serial.printf("E907 - Length too long %d\r\n", temp);
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
					Serial.printf("E900 - Failed to parse length %s as %s\r\n", _lengthInHex, check);
				}
			}
			_lengthInHex = "";
			return;
		}

		// Skip non hex characters upper and lower case
		//const string GOOD_HEX = "0123456789ABCDEF";
		if (('0' > b || b > '9') && ('A' > b || b > 'F') && ('a' > b || b > 'f'))
		{
			Serial.printf("E910 - Non Hex number %s\r\n", _lengthInHex.c_str());
			_lengthInHex = "";
			return;
		}

		// Is it too long?
		if (_lengthInHex.length() > MAX_LENGTH_BYTES)
		{
			Serial.printf("E900 - MAX Length HIT %d\r\n", _lengthInHex);
			_lengthInHex = "";
			return;
		}

		// Add to the length
		_lengthInHex += static_cast<char>(b);
	}
};