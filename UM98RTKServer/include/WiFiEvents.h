#pragma once

#include <WiFi.h>
#include "HandyLog.h"

String MakeHostName();

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
void SetupWiFiEvents()
{
	// Setup the wifi events
	WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) 
	{
		switch (event)
		{
		case SYSTEM_EVENT_WIFI_READY:
			Logln("WIFI - Ready");
			break;
		case SYSTEM_EVENT_SCAN_DONE:
			Logln("WIFI - Scan done");
			break;
		case SYSTEM_EVENT_STA_START:
			Logln("WIFI - Start");
			break;
		case SYSTEM_EVENT_STA_STOP:
			Logln("WIFI - Stop");
			break;
		case SYSTEM_EVENT_STA_CONNECTED:
			Logln("WIFI - Connected");
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
			Logln("WIFI - Disconnected");
			break;
		case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
			Logln("WIFI - Auth mode change");
			break;
		case SYSTEM_EVENT_STA_GOT_IP:
			Logln("WIFI - Got IP");
			break;
		case SYSTEM_EVENT_STA_LOST_IP:	
			Logln("WIFI - Lost IP");
			break;
		case SYSTEM_EVENT_STA_BSS_RSSI_LOW:
			Logln("WIFI - BSS RSSI low");
			break;
		case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
			Logln("WIFI - WPS success");
			break;
		case SYSTEM_EVENT_STA_WPS_ER_FAILED:
			Logln("WIFI - WPS failed");
			break;
		case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
			Logln("WIFI - WPS timeout");
			break;
		case SYSTEM_EVENT_STA_WPS_ER_PIN:
			Logln("WIFI - WPS pin");
			break;
		case SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP:
			Logln("WIFI - WPS PBC overlap");
			break;
		case SYSTEM_EVENT_AP_START:
			Logln("WIFI - AP start");
			break;
		case SYSTEM_EVENT_AP_STOP:
			Logln("WIFI - AP stop");
			break;
		case SYSTEM_EVENT_AP_STACONNECTED:
			Logln("WIFI - AP station connected");
			break;
		case SYSTEM_EVENT_AP_STADISCONNECTED:
			Logln("WIFI - AP station disconnected");
			break;
		case SYSTEM_EVENT_AP_STAIPASSIGNED:
			Logln("WIFI - AP station IP assigned");
			break;
		case SYSTEM_EVENT_AP_PROBEREQRECVED:
			Logln("WIFI - AP probe request received");
			break;
		case SYSTEM_EVENT_ACTION_TX_STATUS:
			Logln("WIFI - Action TX status");
			break;
		case SYSTEM_EVENT_ROC_DONE:
			Logln("WIFI - ROC done");
			break;
		case SYSTEM_EVENT_STA_BEACON_TIMEOUT:
			Logln("WIFI - Beacon timeout");
			break;
		case SYSTEM_EVENT_FTM_REPORT:
			Logln("WIFI - FTM report");
			break;
		case SYSTEM_EVENT_GOT_IP6:
			Logln("WIFI - Got IP6");
			break;
		case SYSTEM_EVENT_ETH_START:
			Logln("WIFI - ETH start");
			break;
		case SYSTEM_EVENT_ETH_STOP:
			Logln("WIFI - ETH stop");
			break;
		case SYSTEM_EVENT_ETH_CONNECTED:
			Logln("WIFI - ETH connected");
			break;
		case SYSTEM_EVENT_ETH_DISCONNECTED:
			Logln("WIFI - ETH disconnected");
			break;
		case SYSTEM_EVENT_ETH_GOT_IP:	
			Logln("WIFI - ETH got IP");
			break;
		case SYSTEM_EVENT_ETH_LOST_IP:
			Logln("WIFI - ETH lost IP");
			break;
		case SYSTEM_EVENT_MAX:
			Logln("WIFI - Max");
			break;			
		default:
			Logf("WIFI - %d", event);
			break;
		}
	});
}


///////////////////////////////////////////////////////////////////////////////
/// @brief Maker a unique host name based on the MAC address with Rtk prefix
String MakeHostName()
{
	auto mac = WiFi.macAddress();
	mac.replace(":", "");
	return "Rtk_" + mac;
}