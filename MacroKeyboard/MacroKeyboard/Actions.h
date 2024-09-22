#pragma once

#include <string>

struct Action
{
	/// @brief Key strikes to trigger the command
	std::string Command;

	/// @brief Number of backspaces to run before the macro
	int Backspace;

	/// @brief Keys to strike after the backspaces
	std::string Macro;
};

const Action ActionList[] = {
	{ "*1", 2, "McLovin@phantom.com" },
	{ "*2", 2, "McLovin\nGoogle Drive\n212 Tahakapoa Valley Rd\nOtago, New Zealand" },
	{ "**4", 25, "QuickLogin@1234567898\n" },
	{ "**5", 25, "mclovin\tPassword!\n" }
};

const int ActionListLength = sizeof(ActionList) / sizeof(ActionList[0]);
