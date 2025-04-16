#pragma once
 
 #include "Global.h"
 #include "driver/temp_sensor.h"
 
 #define TEMP_HISTORY_SIZE (24 * 60)	 // 1 day of history at 60 second intervals
 #define TEMP_INTERVAL_MS (60 * 1000) // 1 minute interval
 
 /////////////////////////////////////////////////////////////////////////////
 // History class hold a collection of all the history records
 // .. for easy recording and access
 /////////////////////////////////////////////////////////////////////////////
 class History
 {
 private:
 	char _tempHistory[TEMP_HISTORY_SIZE]; // Array of temperature history
 	unsigned long _timeOfLastTemperature; // Time of last temperature reading
 
 public:
 	History()
 	{
 		// Setup to read the temperature on first loop
 		_timeOfLastTemperature -= TEMP_INTERVAL_MS;
 
 		// Zero out the temperature history
 		for (size_t i = 0; i < TEMP_HISTORY_SIZE; i++)
 			_tempHistory[i] = 0;
 	}
 
 	const char *GetTemperatures() const { return _tempHistory; } // Get the temperature history
 
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
 
 		// Enable temperature sensor (Will fail first time called. Probably cos already on?)
 		if ((temp_sensor_start()) != ESP_OK)
 			Logln("E100 - Failed to start temperature sensor");
 
 		// Get converted sensor data
 		float tsens_out = 0.0;
 		if (temp_sensor_read_celsius(&tsens_out))
 		{
 			Logln("E101 - Failed to read temperature sensor");
 			return 0.0;
 		}
 
 		// Disable the temperature sensor if it is not needed and save the power
 		if (temp_sensor_stop())
 			Logln("E102 - Failed to stop temperature sensor");
 
 		// Shuffle everything down then array
 		for (size_t i = 0; i < TEMP_HISTORY_SIZE - 1; i++)
 			_tempHistory[i] = _tempHistory[i + 1];
 
 		// Save the temperature history once per 60 seconds
 		_tempHistory[TEMP_HISTORY_SIZE - 1] = (char)tsens_out;
 		return tsens_out;
 	}
 };