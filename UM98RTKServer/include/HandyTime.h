#pragma once

#include <WiFi.h>
#include "time.h"
#include "HandyLog.h"


///////////////////////////////////////////////////////////////////////////////
// Time functions
// WARNING : This class is called by logger so do not log yourself
class HandyTime
{
private:
	const char *_ntpServer = "pool.ntp.org";
	long _gmtOffset_sec = 0;
	int _daylightOffset_sec = 0;

	bool _timeSyncEnabled = false;	 // Indicate if time sync is enabled
	unsigned long _lastSyncTime = 0; // Last time we synced the time
	unsigned long _syncInterval = 0; // Default sync interval of 1 hour


public:
	void EnableTimeSync(std::string tzMinutes)
	{
		if (!tzMinutes.empty())
		{
			 try 
			 {
        		 float minutes = std::stof(tzMinutes);
				 _gmtOffset_sec = static_cast<long>(minutes * 60); // Convert minutes to seconds
				 _daylightOffset_sec = 0; // Default to no daylight saving time
				 Logf("Timezone set to %s minutes (%ld seconds)", tzMinutes.c_str(), _gmtOffset_sec);
    		} 
			catch (const std::invalid_argument& e) 
			{
				Logf("Invalid timezone minutes: %s", tzMinutes.c_str());
    		} 
			catch (const std::out_of_range& e) 
			{
				Logf("Timezone minutes out of range: %s", tzMinutes.c_str());
    		}
		}
		_timeSyncEnabled = true;
	}

	////////////////////////////////////////////////////////////////////////////
	// Read the current time and issue a resync if necessary
	bool ReadTime(struct tm *info)
	{
		if (!_timeSyncEnabled)
			return false;

		// Check if a sync is needed
		if (millis() - _lastSyncTime > _syncInterval)
		{
			Logln("Sync NTP time", false);
			configTime(_gmtOffset_sec, _daylightOffset_sec, _ntpServer);
			_lastSyncTime = millis();

			// Try to get the local time
			if (getLocalTime(info))
			{
				Logln("NTP time synced", false);
				_syncInterval = 60 * 60 * 1000; // Set sync interval to 1 hour
				return true;
			}
			else
			{
				Logln("ERROR : Failed to sync NTP time", false);
				_syncInterval = 60 * 1000; // Set sync interval to 1 minute
				return false;
			}
		}

		// Try to get the local time
		if (getLocalTime(info))
			return true; 

		// If we failed to get the local time, try again after a short delay
		// .. program runs real slow here
		_syncInterval = 60 * 1000; // Set sync interval to 1 minute
		return false;			   // Failed to get local time
	}

	///////////////////////////////////////////////////////////////////////////
	// Get a formatted time
	std::string	LongString()
	{
		struct tm timeinfo;
		if (!ReadTime(&timeinfo))
			return Uptime(millis());

		// 2025-06-20 22:35:54
		const int len = 22;
		char time[len];
		if (strftime(time, len, "%Y-%02m-%02d %H:%M:%S", &timeinfo) == 0)
			return std::string("Error formatting time");
		return std::string(time);
	}

	///////////////////////////////////////////////////////////////////////////
	// Get a compressed format like 2025-06-22 435544
	std::string	FileSafe()
	{
		struct tm timeinfo;
		if (!ReadTime(&timeinfo))
			return Uptime(millis());

		// 2025-06-20 22:35:54
		const int len = 22;
		char time[len];
		if (strftime(time, len, "%Y-%02m-%02d %H%M%S", &timeinfo) == 0)
			return std::string("Error formatting time");
		return std::string(time);
	}

	///////////////////////////////////////////////////////////////////////////
	// Get a short time string HH:MM:SS
	std::string HH_MM_SS()
	{
		struct tm timeinfo;
		if (!ReadTime(&timeinfo))
			return Uptime(millis());

		// 22:35:54
		const int len = 10;
		char time[len];
		if (strftime(time, len, "%H:%M:%S", &timeinfo) == 0)
			return std::string("Error formatting time");
		return std::string(time);
	}

	///////////////////////////////////////////////////////////////////////////
	// Get a short time string HH:MM:SS
	std::string ddd_HH_MM_SS()
	{
		struct tm timeinfo;
		if (!ReadTime(&timeinfo))
			return Uptime(millis());

		// Wed 22:35:54
		const int len = 15;
		char time[len];
		if (strftime(time, len, "%a %H:%M:%S", &timeinfo) == 0)
			return std::string("Error formatting time");
		return std::string(time);
	}
};
