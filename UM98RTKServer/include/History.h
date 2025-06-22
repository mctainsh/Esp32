#pragma once

#include "Global.h"
#ifdef T_DISPLAY_S3
#include "driver/temp_sensor.h"
#endif
#ifdef T_DISPLAY_S2
// #include "C:\Users\john\.platformio\packages\framework-arduinoespressif32\tools\sdk\esp32s2\include\driver\esp32s2\include\driver\temp_sensor.h"
// #include "driver/temperature_sensor.h"
#endif

#define TEMP_HISTORY_SIZE (24 * 60)	 // 1 day of history at 60 second intervals
#define TEMP_INTERVAL_MS (60 * 1000) // 1 minute interval
#define AVERAGE_SEND_TIMERS 300		 // Number of items in the averaging buffer for send time calculation

/////////////////////////////////////////////////////////////////////////////
// History class hold a collection of all the history records
// .. for easy recording and access
/////////////////////////////////////////////////////////////////////////////
class History
{
private:
	// Temperature history
	char _tempHistory[TEMP_HISTORY_SIZE]; // Array of temperature history
	unsigned long _timeOfLastTemperature; // Time of last temperature reading

	// Ntrip send history
	std::vector<int> _sendMicroSeconds[RTK_SERVERS]; // Collection of send times
	unsigned int _lastMeanTimer[RTK_SERVERS];		 // Last mean time for each server
	long _totalSendTimes[RTK_SERVERS];				 // Total send times for each server
	long _totalSendCount[RTK_SERVERS];				 // Total send count for each server

public:
	History()
	{
		// Setup to read the temperature on first loop
		_timeOfLastTemperature -= TEMP_INTERVAL_MS;

		// Zero out the temperature history
		for (size_t i = 0; i < TEMP_HISTORY_SIZE; i++)
			_tempHistory[i] = 0;

		// Reserve space for NTRIP lists
		for (int i = 0; i < RTK_SERVERS; i++)
			_sendMicroSeconds[i].reserve(AVERAGE_SEND_TIMERS);
	}

	const char *GetTemperatures() const { return _tempHistory; } // Get the temperature history
	inline const std::vector<int> &GetNtripSendTime(int index) const { return _sendMicroSeconds[index]; }

	/////////////////////////////////////////////////////////////////////////////////
	// Check the temperature sensor and return the temperature in Celsius
	// .. If the temperature sensor is not available, return 0.0
	// .. Only check every 60 seconds
	float CheckTemperatureLoop()
	{
		// Check if it is time for a new temperature reading
		if (millis() - _timeOfLastTemperature < TEMP_INTERVAL_MS)
			return _tempHistory[TEMP_HISTORY_SIZE - 1];

		_timeOfLastTemperature = millis();
		float tsens_out = 0.0;

#ifdef T_DISPLAY_S3
		// Enable temperature sensor (Will fail first time called. Probably cos already on?)
		if ((temp_sensor_start()) != ESP_OK)
			Logln("E100 - Failed to start temperature sensor");

		// Get converted sensor data
		if (temp_sensor_read_celsius(&tsens_out))
		{
			Logln("E101 - Failed to read temperature sensor");
			return 0.0;
		}

		// Disable the temperature sensor if it is not needed and save the power
		if (temp_sensor_stop())
			Logln("E102 - Failed to stop temperature sensor");
#endif

		// Shuffle everything down then array
		for (size_t i = 0; i < TEMP_HISTORY_SIZE - 1; i++)
			_tempHistory[i] = _tempHistory[i + 1];

		// Save the temperature history once per 60 seconds
		_tempHistory[TEMP_HISTORY_SIZE - 1] = (char)tsens_out;
		return tsens_out;
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Add a send time to the history
	// .. This is the time it took to send the data to the NTRIP server
	// .. This is used to calculate the average send time
	// .. and the maximum send time
	void AddNtripSendTime(int index, int time)
	{
		if (0 > index || index >= RTK_SERVERS)
			return;

		// For a timeout of the for storing the mean value
		_totalSendTimes[index] += time;
		_totalSendCount[index]++;
		if (millis() - _lastMeanTimer[index] < 1000)
			return;

		// Calculate the mean value
		int mean = _totalSendTimes[index] / _max(_totalSendCount[index], 1);

		// Reset the total send times and count
		_totalSendTimes[index] = 0;
		_totalSendCount[index] = 0;
		_lastMeanTimer[index] = millis();

		// Add the new mean value
		_sendMicroSeconds[index].push_back(mean);
		while (_sendMicroSeconds[index].size() > AVERAGE_SEND_TIMERS)
			_sendMicroSeconds[index].erase(_sendMicroSeconds[index].begin());
	}

	///////////////////////////////////////////////////////////////////////////////
	// Get the Median send time without sorting the list
	int MedianSendTime(int index)
	{
		if (0 > index || index >= RTK_SERVERS)
			return 0;
		if (_sendMicroSeconds[index].size() < 1)
			return 0;
		// for (int n : _sendMicroSeconds[index])
		// 	total += n;
		// return total / _sendMicroSeconds[index].size();

		// Make a sorted copy of the list
		std::vector<int> sortedList = _sendMicroSeconds[index];
		std::sort(sortedList.begin(), sortedList.end());

		// Get the median value
		int medianIndex = sortedList.size() / 2;
		if (sortedList.size() % 2 == 0)
			return (sortedList[medianIndex - 1] + sortedList[medianIndex]) / 2;
		else
			return sortedList[medianIndex];
	}
};