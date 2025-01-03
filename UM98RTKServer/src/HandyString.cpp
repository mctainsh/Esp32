#include <memory>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <WiFi.h>

#include "HandyLog.h"

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
