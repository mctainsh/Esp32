#pragma once

#include <WiFi.h>
#include <string>
#include <vector>

template<typename... Args>
std::string StringPrintf(const std::string& format, Args... args);

bool StartsWith(const std::string& fullString, const std::string& startString);
bool StartsWith(const char* szA, const char* szB);
std::vector<std::string> Split(const std::string& s, const std::string delimiter);
void FormatNumber(int number, int width, std::string& result);
bool EndsWith(const std::string& fullString, const std::string& ending);
bool IsValidHex(const std::string& str);
bool IsValidDouble(const char* str, double *pVal);
const char* WifiStatus(wl_status_t status);

#include "HandyString.tpp"