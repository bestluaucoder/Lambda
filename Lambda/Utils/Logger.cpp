#include "Logger.h"

#include <Windows.h>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <mutex>

static std::ofstream g_file;
static std::mutex    g_mutex;
static std::string   g_log_path;

static std::string GetLogsDir() {
    const char* profile = getenv("USERPROFILE");
    std::string docs = profile ? std::string(profile) + "\\Documents" : ".";
    return docs + "\\Lambda\\Logs";
}

static std::string Timestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << st.wYear  << "-"
       << std::setw(2) << st.wMonth << "-"
       << std::setw(2) << st.wDay   << " "
       << std::setw(2) << st.wHour  << ":"
       << std::setw(2) << st.wMinute << ":"
       << std::setw(2) << st.wSecond;
    return ss.str();
}

static std::string LogFileName() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << st.wYear  << "-"
       << std::setw(2) << st.wMonth << "-"
       << std::setw(2) << st.wDay   << "_"
       << std::setw(2) << st.wHour  << "-"
       << std::setw(2) << st.wMinute << "-"
       << std::setw(2) << st.wSecond << ".log";
    return ss.str();
}

void Logger::Init() {
    std::error_code ec;
    std::string dir = GetLogsDir();
    std::filesystem::create_directories(dir, ec);

    g_log_path = dir + "\\" + LogFileName();
    g_file.open(g_log_path, std::ios::out | std::ios::trunc);

    if (!g_file.is_open())
        return;

    g_file << "[" << Timestamp() << "] lambda loaded\n";
    g_file.flush();
}

void Logger::Write(const std::string& msg) {
    if (!g_file.is_open()) return;
    std::lock_guard<std::mutex> lk(g_mutex);

    std::string clean;
    bool skip = false;
    int digits = 0;

    for (char c : msg) {
        if (c == '\a') { skip = true; digits = 0; continue; }
        if (skip) {
            if (++digits == 6) skip = false;
            continue;
        }
        if (c == '\n') continue;
        clean += c;
    }

    g_file << "[" << Timestamp() << "] " << clean << "\n";
    g_file.flush();
}

void Logger::WriteCrash(const std::string& msg) {
    if (!g_file.is_open()) return;
    std::lock_guard<std::mutex> lk(g_mutex);
    g_file << "\n[" << Timestamp() << "] *** CRASH ***\n" << msg << "\n";
    g_file.flush();
}

void Logger::Shutdown() {
    if (!g_file.is_open()) return;
    std::lock_guard<std::mutex> lk(g_mutex);
    g_file << "[" << Timestamp() << "] lambda unloaded\n";
    g_file.close();
}
