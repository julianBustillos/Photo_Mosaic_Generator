#pragma once

#include "WindowsSafe.h"
#include <string>
#include <filesystem>

namespace SystemUtils
{
    inline std::string getCurrentProcessDirectory()
    {
        char buffer[MAX_PATH];
        GetModuleFileName(NULL, buffer, sizeof(buffer));
        return std::filesystem::path(buffer).parent_path().string();
    }
}