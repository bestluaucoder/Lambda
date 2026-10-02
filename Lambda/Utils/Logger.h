#pragma once

#include <string>

namespace Logger {
    void Init();
    void Write(const std::string& msg);
    void WriteCrash(const std::string& msg);
    void Shutdown();
}
