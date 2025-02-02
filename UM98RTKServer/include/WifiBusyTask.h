#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "Global.h"
#include "HandyString.h"
#include "MyDisplay.h"

#define DISPLAY_PAGE 0
#define DISPLAY_ROW 0

extern WiFiManager _wifiManager;

///////////////////////////////////////////////////////////////////////////
// Class create a task to update the display showing a count down during
// .. wifi connection. This allows the autoconnect to have a time out
// .. and still see how much we have to go
// NOTE : This is a bit risky as the display is not thread safe. Only
//		  use if display is static
class WifiBusyTask
{
private:
	int _wifiConnectingCountDown = 0;
	TaskHandle_t _connectingTask = NULL;
	MyDisplay &_display;
	SemaphoreHandle_t _xMutex = NULL;

public:
	WifiBusyTask(MyDisplay &display) : _display(display)
	{
		_xMutex = xSemaphoreCreateMutex();
		xTaskCreatePinnedToCore(
			TaskWrapper,
			"WifiBusyCountDown", // Task name
			5000,				 // Stack size (bytes)
			this,				 // Parameter
			1,					 // Task priority
			&_connectingTask,	 // Task handle
			APP_CPU_NUM);
	}

	~WifiBusyTask()
	{
		Serial.println("----- WifiBusyTask Terminating");
		//_display.SetCell(APP_VERSION, DISPLAY_PAGE, DISPLAY_ROW);
		vTaskDelete(_connectingTask);
	}

	static void TaskWrapper(void *param)
	{
		// Cast the parameter back to MyClass and call the member function
		WifiBusyTask *instance = static_cast<WifiBusyTask *>(param);
		instance->TaskFunction();
	}

	///////////////////////////////////////////////////////////////////////////
	// Loop forever displaying count down on screen
	void TaskFunction()
	{
		Serial.println("+++++ WifiBusyTask Starting");
		while (true)
		{
			xSemaphoreTake(_xMutex, portMAX_DELAY);
			_wifiConnectingCountDown--;
			xSemaphoreGive(_xMutex);

//			auto message = StringPrintf("%s -> Retry %ds", APP_VERSION, _wifiConnectingCountDown);
//			auto message = StringPrintf("%s -> Retry %ds", _wifiManager.getWiFiSSID(), _wifiConnectingCountDown);
			auto message = StringPrintf("[R:%d] %s", _wifiConnectingCountDown, WifiStatus(WiFi.status()));

			Serial.println(message.c_str());
			_display.SetCell(message, DISPLAY_PAGE, DISPLAY_ROW);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Called to start the countdown
	void StartCountDown(int duration)
	{
		xSemaphoreTake(_xMutex, portMAX_DELAY);
		_wifiConnectingCountDown = duration;
		xSemaphoreGive(_xMutex);
	}
};
