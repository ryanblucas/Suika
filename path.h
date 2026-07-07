/*
	path.h ~ RL
*/

#pragma once

#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <cstdlib>
#include <Windows.h>
#include <ShlObj.h>
#endif

namespace path
{
	inline std::string find_system_font(std::string_view font)
	{
#ifdef _WIN32
		PWSTR wpath = NULL;
		if (FAILED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &wpath)))
		{
			throw std::runtime_error("Failed to find debug font.");
		}
		auto needed = wcstombs(NULL, wpath, MAX_PATH);
		std::string path(needed, '\0');
		wcstombs(path.data(), wpath, needed);
		path += "\\";
#endif
		return path + std::string(font);
	}
}