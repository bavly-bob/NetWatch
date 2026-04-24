#ifndef PROCESS_MONITOR_HPP
#define PROCESS_MONITOR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdint>

#ifdef _WIN32
  #include <windows.h>
#endif

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();
    
    struct ProcessInfo {
        uint32_t pid;
        std::string name;
        float cpuUsagePercent;
        uint64_t memoryUsageKB;
    };
    
    std::vector<ProcessInfo> getProcessList() const;
    ProcessInfo getProcessInfo(uint32_t pid) const;

    void start(std::chrono::milliseconds interval = std::chrono::milliseconds(2000));
    void stop();
    
private:

    std::vector<ProcessInfo> sampleProcesses();

#ifdef _WIN32
    struct PrevTimes {
	    ULONGLONG kernel{0};
	    ULONGLONG user{0};
	    ULONGLONG wallClock{0};
	    int numCores{1};
    };
#else
    struct PrevTimes {
	    unsigned long long utime{0};
	    unsigned long long stime{0};
	    unsigned long long wallClock{0};
	    long clockTicks{100};
    };
#endif

    std::unordered_map<uint32_t, PrevTimes> m_prevTimes;

    mutable std::mutex m_mutex;
    std::vector<ProcessInfo> m_processes;

    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::chrono::milliseconds m_interval{2000};

    void pollLoop();
};

#endif