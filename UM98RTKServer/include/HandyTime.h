#pragma once

#include <WiFi.h>
#include "time.h"
#include <esp_sntp.h> // Include SNTP for time synchronization

#include "HandyLog.h"
#include "HandyString.h"

#define TIME_SYNC_SHORT 60 * 1000		   // 1 minute
#define TIME_SYNC_LONG 12 * 60 * 60 * 1000 // 12 hours

class HandyTime;
extern HandyTime _handyTime;

///////////////////////////////////////////////////////////////////////////////
// Time functions
// WARNING : This class is called by logger so do not log yourself
class HandyTime
{
private:
	long _gmtOffset_sec = 0;
	int _daylightOffset_sec = 0;

	bool _timeSyncEnabled = false;	 // Indicate if time sync is enabled
	unsigned long _lastSyncTime = 0; // Last time we synced the time
	unsigned long _syncInterval = 0; // Default sync interval of 1 hour
	bool _gotGoodTime = false;		 // Indicate if we got a good time from NTP (Start logging)

public:
	const bool GotGoodTime() const { return _gotGoodTime; }

	void EnableTimeSync(std::string tzMinutes)
	{
		Logln("Enable Time Sync...");
		if (!tzMinutes.empty())
		{
			try
			{
				float minutes = std::stof(tzMinutes);
				_gmtOffset_sec = static_cast<long>(minutes * 60); // Convert minutes to seconds
				_daylightOffset_sec = 0;						  // Default to no daylight saving time
				Logf("\tTimezone set to %s minutes (%ld seconds)", tzMinutes.c_str(), _gmtOffset_sec);
			}
			catch (const std::invalid_argument &e)
			{
				Logf("\tInvalid timezone minutes: %s", tzMinutes.c_str());
			}
			catch (const std::out_of_range &e)
			{
				Logf("\tTimezone minutes out of range: %s", tzMinutes.c_str());
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
			if (WiFi.status() == WL_CONNECTED)
			{

				_lastSyncTime = millis();
				_syncInterval = TIME_SYNC_SHORT;

				// Register the SNTP sync callback
				sntp_set_time_sync_notification_cb(HandyTime::OnTimeSyncCallback);

				// This is a non-blocking call
				configTime(_gmtOffset_sec, _daylightOffset_sec, "pool.ntp.org", "time.nist.gov", "time.google.com");

				// Try to get the local time
				// if (getLocalTime(info))
				// {
				// 	_gotGoodTime = true; // We got a good time
				// 	Logln("NTP time synced", false);
				// 	_syncInterval = TIME_SYNC_LONG
				// 	return true;
				// }
				// else
				// {
				// 	Logln("ERROR : Failed to sync NTP time", false);
				// 	return false;
				// }
			}
		}

		// Try to get the local time
		if (_gotGoodTime && getLocalTime(info))
			return true;

		// If we failed to get the local time, try again after a short delay
		// .. program runs real slow here
		_syncInterval = TIME_SYNC_SHORT;
		return false; // Failed to get local time
	}

	//////////////////////////////////////////////////////////////////
	// Callback function for time synchronization
	// .. This is because the config time function does not block
	// .. and we need to wait for the time to be set
	static void OnTimeSyncCallback(struct timeval *tv)
	{
		Logln(StringPrintf("Time synchronized via NTP %u", tv->tv_sec).c_str(), false);
		if (tv == nullptr || tv->tv_sec == 0)
		{
			Logln("*** ERROR : HandyTime OnTimeSyncCallback failed", false);
			return;
		}
		tm info;
		if (!getLocalTime(&info))
			return;
		Logln("NTP time synced", false);

		_handyTime._syncInterval = TIME_SYNC_LONG;
		_handyTime._gotGoodTime = true;
	}

	///////////////////////////////////////////////////////////////////////////
	// Get a formatted time
	std::string LongString()
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
	std::string FileSafe()
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
