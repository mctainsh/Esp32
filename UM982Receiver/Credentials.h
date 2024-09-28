#pragma once

///////////////////////////////////////////////////////////////////////////////
// Keep credentials here.
// Do not check into source control

// Your network SSID (name)
const char* WIFI_SSID = "Im_Watching_You";		   		

// Your network password
const char* WIFI_PASSWORD = "MyPassword";  
 
 // Bearer for RTK server (Format is username:password)
 const char* RTK_USERNAME_PASSWORD = "HARRY:Secret";

 // RTK Connection details
 const char* RTF_SERVER_ADDRESS = "ntrip.data.gnss.ga.gov.au";
 const int RTF_SERVER_PORT = 2101;
 const char* RTF_MOUNT_POINT = "CLEV00AUS0";	//  "PTHL00AUS0"      // Port headland
