
#include <memory>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <WiFi.h>

#include "HandyLog.h"

bool StartsWith(const std::string& fullString, const std::string& startString)
{
	if (startString.length() > fullString.length())
		return false;
	return std::equal(startString.begin(), startString.end(), fullString.begin());
}

bool StartsWith(const char* szA, const char* szB)
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
// Split the string
std::vector<std::string> Split(const std::string& s, const std::string delimiter)
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
void FormatNumber(int number, int width, std::string& result)
{
	std::ostringstream oss;
	oss << std::setw(width) << std::setfill('0') << number;
	result = oss.str();
}


bool EndsWith(const std::string& fullString, const std::string& ending)
{
	if (ending.size() > fullString.size())
		return false;
	return std::equal(ending.rbegin(), ending.rend(), fullString.rbegin());
}

bool IsValidHex(const std::string& str)
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

bool IsValidDouble(const char* str, double *pVal)
{
	if (str == NULL || *str == '\0')
		return false;

	char* endptr;
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
const char* WifiStatus(wl_status_t status)
{
	switch (status)
	{
		case WL_NO_SHIELD: return "No shield";
		case WL_IDLE_STATUS: return "Idle status";
		case WL_NO_SSID_AVAIL: return "No SSID Avail.";
		case WL_SCAN_COMPLETED: return "Scan Complete";
		case WL_CONNECTED: return "Connected";
		case WL_CONNECT_FAILED: return "Connect failed";
		case WL_CONNECTION_LOST: return "Lost connection";
		case WL_DISCONNECTED: return "Disconnected";
	}
	return "Unknown";
}