#pragma once

#include <string>

void Log(const char *msg);
void Logln(const char *msg);

template<typename... Args>
void Logf(const std::string& format, Args... args);

#include "HandyLog.tpp"
