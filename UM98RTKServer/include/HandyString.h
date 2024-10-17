#pragma once

#include <memory>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Taken from
//	https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
// Answer by iFreiticht
template<typename... Args>
std::string StringPrintf(const std::string& format, Args... args)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;  // Extra space for '\0'
	if (size_s <= 0)
		throw std::runtime_error("Unable to allocate string size.");
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), args...);
	return std::string(buf.get(), buf.get() + size - 1);  // We don't want the '\0' inside
}

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

///////////////////////////////////////////////////////////////////////////////
// Verify the check sum in the NMEA sentence
bool VerifyChecksum(const std::string& nmeaSentence)
{
	// Check starts with $
	if (nmeaSentence.length() < 1)
	{
		Logln("\tE100 - Too short");
		return false;
	}
	if (nmeaSentence[0] != '$' && nmeaSentence[0] != '#')
	{
		Logln("\tE101 = Missing '$'");
		return false;
	}

	// Find the positions of the '$' and '*'
	size_t endPos = nmeaSentence.find('*');
	if (endPos == std::string::npos)
	{
		Logln("\tE102 - Missing '*'");
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
		Logf("\tE101 %02x != %02x\r\n", calculatedChecksum, providedChecksum);
		return false;
	}
	return true;
}

const double EARTH_RADIUS = 6371000.0;	// Earth's radius in meters

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