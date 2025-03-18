#include <memory>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <WiFi.h>

#include "HandyLog.h"
#include "HandyString.h"

bool StartsWith(const std::string &fullString, const std::string &startString)
{
	if (startString.length() > fullString.length())
		return false;
	return std::equal(startString.begin(), startString.end(), fullString.begin());
}

bool StartsWith(const char *szA, const char *szB)
{
	while (*szA && *szB)
	{
		if (*szA != *szB)
			return false;
		szA++;
		szB++;
	}
	return *szB == '\0';
}

///////////////////////////////////////////////////////////////////////////
/// @brief Format a the int with thousand separators
/// @param number
/// @return 1,234,567
std::string ToThousands(int number)
{
	std::string value = std::to_string(number);
	int len = value.length();
	int dlen = 3;

	while (len > dlen)
	{
		value.insert(len - dlen, 1, ',');
		dlen += 4;
		len += 1;
	}
	return value;

	// std::stringstream ss;
	// ss.imbue(std::locale("en_US.UTF-8"));
	//	 ss << std::fixed << number;
	// return ss.str();
}

///////////////////////////////////////////////////////////////////////////
/// @brief Convert a array of bytes to string of hex numbers
std::string HexDump( const unsigned char *data, int len )
{
	std::string hex;
	for (int n = 0; n < len; n++)
	{
		hex += StringPrintf("%02x ", data[n]);
	}
	return hex;
}

///////////////////////////////////////////////////////////////////////////
/// @brief Convert a array of bytes to string of hex numbers and ASCII characters
/// Format: "00 01 02 03 04 05 06 07-08 09 0a 0b 0c 0d 0e 0f 0123456789abcdef"
std::string HexAsciDump(const unsigned char* data, int len)
{
	if (len == 0)
		return "";
	std::string lines;
	const int SIZE = 64;
	char szText[SIZE + 1];
	for (int n = 0; n < len; n++)
	{
		auto index = n % 16;
		if (index == 0)
		{
			szText[SIZE - 1] = '\0';
			if (n > 0)
				lines += (std::string(szText) + "\r\n");

			// Fill the szText with spaces
			memset(szText, ' ', SIZE);
		}

		// Add the hex value to szText as position 3 * n
		auto offset = 3 * index;
		snprintf(szText + offset, 3, "%02x", data[n]);
		szText[offset + 2] = ' ';
		szText[3 * 16 + 1 + index] = data[n] < 0x20 ? 0xfa : data[n];
	}
	szText[SIZE - 1] = '\0';
	lines += std::string(szText);
	return lines;
}

///////////////////////////////////////////////////////////////////////////
// Split the string
std::vector<std::string> Split(const std::string &s, const std::string delimiter)
{
	size_t pos_start = 0;
	size_t pos_end;
	size_t delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
	{
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

///////////////////////////////////////////////////////////////////////////////
void FormatNumber(int number, int width, std::string &result)
{
	std::ostringstream oss;
	oss << std::setw(width) << std::setfill('0') << number;
	result = oss.str();
}

bool EndsWith(const std::string &fullString, const std::string &ending)
{
	if (ending.size() > fullString.size())
		return false;
	return std::equal(ending.rbegin(), ending.rend(), fullString.rbegin());
}

bool IsValidHex(const std::string &str)
{
	if (str.empty())
		return false;
	for (char c : str)
	{
		if (!std::isxdigit(c))
			return false;
	}
	return true;
}

bool IsValidDouble(const char *str, double *pVal)
{
	if (str == NULL || *str == '\0')
		return false;

	char *endptr;
	*pVal = strtod(str, &endptr);

	// Check if the entire string was converted
	if (*endptr != '\0')
		return false;

	// Check for special cases like NaN and infinity
	if (*pVal == 0.0 && endptr == str)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Convert Wifi Status to const char*
const char *WifiStatus(wl_status_t status)
{
	switch (status)
	{
	case WL_NO_SHIELD:
		return "No shield";
	case WL_IDLE_STATUS:
		return "Idle status";
	case WL_NO_SSID_AVAIL:
		return "No SSID Avail.";
	case WL_SCAN_COMPLETED:
		return "Scan Complete";
	case WL_CONNECTED:
		return "Connected";
	case WL_CONNECT_FAILED:
		return "Connect failed";
	case WL_CONNECTION_LOST:
		return "Lost connection";
	case WL_DISCONNECTED:
		return "Disconnected";
	}
	return "Unknown";
}

std::string ReplaceNewlineWithTab(const std::string &input)
{
	std::string output;
	for (char c : input)
	{
		if (c == '\n')
			output += '\t';
		else
			output += c;
	}
	return output;
}

std::string Replace(const std::string &input, const std::string &search, const std::string &replace)
{
	std::string result = input;
	std::size_t pos = 0;
	while ((pos = result.find(search, pos)) != std::string::npos)
	{
		result.replace(pos, search.length(), replace);
		pos += replace.length(); // Move past the newly inserted characters
	}
	return result;
}

void RemoveLastLfCr(std::string &str)
{
    if (str.size() >= 2 && str.compare(str.size() - 2, 2, "\r\n") == 0)
        str.erase(str.size() - 2, 2);
}

void ReplaceCrLfEncode(std::string &str)
{
    std::string crlf = "\r\n";
    std::string newline = "\\r\\n";
    size_t pos = 0;

    while ((pos = str.find(crlf, pos)) != std::string::npos)
    {
        str.replace(pos, crlf.length(), newline);
        pos += newline.length();
	}
}
    

std::string Base64Encode(const std::string &input)
{
	static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string encoded;
	encoded.reserve(((input.size() + 2) / 3) * 4);

	uint32_t temp;
	int bits_collected = 0;

	for (auto ch : input)
	{
		temp <<= 8;
		temp |= ch & 0xFF;
		bits_collected += 8;

		while (bits_collected >= 6)
		{
			encoded.push_back(lookup[(temp >> (bits_collected - 6)) & 0x3F]);
			bits_collected -= 6;
		}
	}

	if (bits_collected > 0)
	{
		temp <<= 6 - bits_collected;
		encoded.push_back(lookup[temp & 0x3F]);
	}

	while (encoded.size() % 4 != 0)
		encoded.push_back('=');

	return encoded;
}


///////////////////////////////////////////////////////////////////////////////
// Verify the check sum in the NMEA sentence
bool VerifyChecksum(const std::string &nmeaSentence)
{
	// Check starts with $
	if (nmeaSentence.length() < 1)
	{
		Serial.println("\tE100 - Too short");
		return false;
	}
	if (nmeaSentence[0] != '$' && nmeaSentence[0] != '#')
	{
		Serial.println("\tE101 = Missing '$'");
		return false;
	}

	// Find the positions of the '$' and '*'
	size_t endPos = nmeaSentence.find('*');
	if (endPos == std::string::npos)
	{
		Serial.println("\tE102 - Missing '*'");
		return false;
	}

	// Extract the checksum from the sentence
	std::string checksumStr = nmeaSentence.substr(endPos + 1, 2);
	int providedChecksum;
	std::stringstream ss;
	ss << std::hex << checksumStr;
	ss >> providedChecksum;

	// Calculate the checksum
	int calculatedChecksum = 0;
	for (size_t i = 1; i < endPos; i++)
		calculatedChecksum ^= nmeaSentence[i];

	if (calculatedChecksum != providedChecksum)
	{
		Serial.printf("\tE103 Checksum %02x != %02x\r\n", calculatedChecksum, providedChecksum);
		return false;
	}
	return true;
}


const double EARTH_RADIUS = 6371000.0; // Earth's radius in meters

// Convert degrees to radians
double degreesToRadians(double degrees)
{
	return degrees * M_PI / 180.0;
}

///////////////////////////////////////////////////////////////////////////////
// Calculate the distance between two latitude and longitude points
double HaversineMetres(double lat1, double lon1, double lat2, double lon2)
{
	// Convert latitude and longitude from degrees to radians
	lat1 = degreesToRadians(lat1);
	lon1 = degreesToRadians(lon1);
	lat2 = degreesToRadians(lat2);
	lon2 = degreesToRadians(lon2);

	// Haversine formula
	double dlat = lat2 - lat1;
	double dlon = lon2 - lon1;
	double a = std::sin(dlat / 2) * std::sin(dlat / 2) + std::cos(lat1) * std::cos(lat2) * std::sin(dlon / 2) * std::sin(dlon / 2);
	double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
	double distance = EARTH_RADIUS * c;

	return distance;
}
