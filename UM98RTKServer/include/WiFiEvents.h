#pragma once

#include <WiFi.h>
#include "HandyLog.h"

String MakeHostName();

const char *_lastEvent = nullptr; // Pointer to store the last event message
int _duplicateEventCount = 0;	  // Counter for duplicate events

void LogDuplicateEvents()
{
	if (_lastEvent == nullptr)
		return;
	if (_duplicateEventCount == 1)
	{
		Logf(_lastEvent);
	}
	else
	{
		Logf(" >>> %s (%d times)", _lastEvent, _duplicateEventCount);
	}
	_lastEvent = nullptr;	  // Reset the last event after logging
	_duplicateEventCount = 0; // Reset the duplicate count
}

////////////////////////////////////////////////////////////////////////////////
// Setup the wifi events
// SYSTEM_EVENT_WIFI_READY = 0,           /*!< ESP32 WiFi ready */
// SYSTEM_EVENT_SCAN_DONE,                /*!< ESP32 finish scanning AP */
// SYSTEM_EVENT_STA_START,                /*!< ESP32 station start */
// SYSTEM_EVENT_STA_STOP,                 /*!< ESP32 station stop */
// SYSTEM_EVENT_STA_CONNECTED,            /*!< ESP32 station connected to AP */
// SYSTEM_EVENT_STA_DISCONNECTED,         /*!< ESP32 station disconnected from AP */
// SYSTEM_EVENT_STA_AUTHMODE_CHANGE,      /*!< the auth mode of AP connected by ESP32 station changed */
// SYSTEM_EVENT_STA_GOT_IP,               /*!< ESP32 station got IP from connected AP */
// SYSTEM_EVENT_STA_LOST_IP,              /*!< ESP32 station lost IP and the IP is reset to 0 */
// SYSTEM_EVENT_STA_BSS_RSSI_LOW,         /*!< ESP32 station connected BSS rssi goes below threshold */
// SYSTEM_EVENT_STA_WPS_ER_SUCCESS,       /*!< ESP32 station wps succeeds in enrollee mode */
// SYSTEM_EVENT_STA_WPS_ER_FAILED,        /*!< ESP32 station wps fails in enrollee mode */
// SYSTEM_EVENT_STA_WPS_ER_TIMEOUT,       /*!< ESP32 station wps timeout in enrollee mode */
// SYSTEM_EVENT_STA_WPS_ER_PIN,           /*!< ESP32 station wps pin code in enrollee mode */
// SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP,   /*!< ESP32 station wps overlap in enrollee mode */
// SYSTEM_EVENT_AP_START,                 /*!< ESP32 soft-AP start */
// SYSTEM_EVENT_AP_STOP,                  /*!< ESP32 soft-AP stop */
// SYSTEM_EVENT_AP_STACONNECTED,          /*!< a station connected to ESP32 soft-AP */
// SYSTEM_EVENT_AP_STADISCONNECTED,       /*!< a station disconnected from ESP32 soft-AP */
// SYSTEM_EVENT_AP_STAIPASSIGNED,         /*!< ESP32 soft-AP assign an IP to a connected station */
// SYSTEM_EVENT_AP_PROBEREQRECVED,        /*!< Receive probe request packet in soft-AP interface */
// SYSTEM_EVENT_ACTION_TX_STATUS,         /*!< Receive status of Action frame transmitted */
// SYSTEM_EVENT_ROC_DONE,                 /*!< Indicates the completion of Remain-on-Channel operation status */
// SYSTEM_EVENT_STA_BEACON_TIMEOUT,       /*!< ESP32 station beacon timeout */
// SYSTEM_EVENT_FTM_REPORT,               /*!< Receive report of FTM procedure */
// SYSTEM_EVENT_GOT_IP6,                  /*!< ESP32 station or ap or ethernet interface v6IP addr is preferred */
// SYSTEM_EVENT_ETH_START,                /*!< ESP32 ethernet start */
// SYSTEM_EVENT_ETH_STOP,                 /*!< ESP32 ethernet stop */
// SYSTEM_EVENT_ETH_CONNECTED,            /*!< ESP32 ethernet phy link up */
// SYSTEM_EVENT_ETH_DISCONNECTED,         /*!< ESP32 ethernet phy link down */
// SYSTEM_EVENT_ETH_GOT_IP,               /*!< ESP32 ethernet got IP from connected AP */
// SYSTEM_EVENT_ETH_LOST_IP,              /*!< ESP32 ethernet lost IP and the IP is reset to 0 */
// SYSTEM_EVENT_MAX                       /*!< Number of members in this enum */
void OnWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
	const char *eventMessage = nullptr;
	switch (event)
	{
	case SYSTEM_EVENT_WIFI_READY:
		eventMessage = "WIFI - Ready";
		break;
	case SYSTEM_EVENT_SCAN_DONE:
		eventMessage = "WIFI - Scan done";
		break;
	case SYSTEM_EVENT_STA_START:
		eventMessage = "WIFI - Start";
		break;
	case SYSTEM_EVENT_STA_STOP:
		eventMessage = "WIFI - Stop";
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		eventMessage = "WIFI - Connected";
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		eventMessage = "WIFI - Disconnected";
		break;
	case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
		eventMessage = "WIFI - Auth mode change";
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		eventMessage = "WIFI - Got IP";
		break;
	case SYSTEM_EVENT_STA_LOST_IP:
		eventMessage = "WIFI - Lost IP";
		break;
	case SYSTEM_EVENT_STA_BSS_RSSI_LOW:
		eventMessage = "WIFI - BSS RSSI low";
		break;
	case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
		eventMessage = "WIFI - WPS success";
		break;
	case SYSTEM_EVENT_STA_WPS_ER_FAILED:
		eventMessage = "WIFI - WPS failed";
		break;
	case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
		eventMessage = "WIFI - WPS timeout";
		break;
	case SYSTEM_EVENT_STA_WPS_ER_PIN:
		eventMessage = "WIFI - WPS pin";
		break;
	case SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP:
		eventMessage = "WIFI - WPS PBC overlap";
		break;
	case SYSTEM_EVENT_AP_START:
		eventMessage = "WIFI - AP start";
		break;
	case SYSTEM_EVENT_AP_STOP:
		eventMessage = "WIFI - AP stop";
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		eventMessage = "WIFI - AP station connected";
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		eventMessage = "WIFI - AP station disconnected";
		break;
	case SYSTEM_EVENT_AP_STAIPASSIGNED:
		eventMessage = "WIFI - AP station IP assigned";
		break;
	case SYSTEM_EVENT_AP_PROBEREQRECVED:
		eventMessage = "WIFI - AP probe request received";
		break;
	case SYSTEM_EVENT_ACTION_TX_STATUS:
		eventMessage = "WIFI - Action TX status";
		break;
	case SYSTEM_EVENT_ROC_DONE:
		eventMessage = "WIFI - ROC done";
		break;
	case SYSTEM_EVENT_STA_BEACON_TIMEOUT:
		eventMessage = "WIFI - Beacon timeout";
		break;
	case SYSTEM_EVENT_FTM_REPORT:
		eventMessage = "WIFI - FTM report";
		break;
	case SYSTEM_EVENT_GOT_IP6:
		eventMessage = "WIFI - Got IP6";
		break;
	case SYSTEM_EVENT_ETH_START:
		eventMessage = "WIFI - ETH start";
		break;
	case SYSTEM_EVENT_ETH_STOP:
		eventMessage = "WIFI - ETH stop";
		break;
	case SYSTEM_EVENT_ETH_CONNECTED:
		eventMessage = "WIFI - ETH connected";
		break;
	case SYSTEM_EVENT_ETH_DISCONNECTED:
		eventMessage = "WIFI - ETH disconnected";
		break;
	case SYSTEM_EVENT_ETH_GOT_IP:
		eventMessage = "WIFI - ETH got IP";
		break;
	case SYSTEM_EVENT_ETH_LOST_IP:
		eventMessage = "WIFI - ETH lost IP";
		break;
	case SYSTEM_EVENT_MAX:
		eventMessage = "WIFI - Max";
		break;
	default:
		LogDuplicateEvents();
		Logf("WIFI - %d", event);
		return;
	}

	// Don't log duplicated events
	if (_lastEvent == eventMessage)
	{
		_duplicateEventCount++;
		if (_duplicateEventCount > 50)
			LogDuplicateEvents();
	}
	else
	{
		LogDuplicateEvents();
		Logln(eventMessage);
		_lastEvent = eventMessage; // Store the last event message
		_duplicateEventCount = 1;  // Reset the duplicate count
	}
}

void SetupWiFiEvents()
{
	// Setup the wifi events
	WiFi.onEvent(OnWifiEvent);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Maker a unique host name based on the MAC address with Rtk prefix
String MakeHostName()
{
	auto mac = WiFi.macAddress();
	mac.replace(":", "");
	return "Rtk_" + mac;
}