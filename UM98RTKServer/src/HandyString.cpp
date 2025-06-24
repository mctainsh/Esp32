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
std::string HexDump(const unsigned char *data, int len)
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
std::string HexAsciDump(const unsigned char *data, int len)
{
	if (len == 0)
		return "";
	std::string lines;
	const int SIZE = 4 * 16 + 1;
	char szText[SIZE + 1];
	for (int n = 0; n < len; n++)
	{
		auto index = n % 16;
		if (index == 0)
		{
			szText[SIZE] = '\0';
			if (n > 0)
				lines += (std::string(szText) + "\r\n");

			// Fill the szText with spaces
			memset(szText, ' ', SIZE);
		}

		// Add the hex value to szText as position 3 * n
		auto offset = 3 * index;
		snprintf(szText + offset, 3, "%02x", data[n]);
		szText[offset + 2] = ' ';

		auto asciiIndex = 3 * 16 + index;
		if (data[n] < 0x20)
			szText[asciiIndex] = 0xB7;
		else
			szText[asciiIndex] = data[n];
	}
	szText[SIZE] = '\0';
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
		output += c;
		if (c == '\n')
			output += '\t';
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

/////////////////////////////////////////////////////////////////////////////////
// Helper function to trim whitespace from both ends of a std::string
std::string Trim(const std::string &str)
{
	size_t first = str.find_first_not_of(" \t\n\r");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t\n\r");
	return str.substr(first, (last - first + 1));
}

/////////////////////////////////////////////////////////////////////////////////
// Make the string lowercase
std::string ToLower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;

}