#ifndef SYSTEM_MONITOR_HPP
#define SYSTEM_MONITOR_HPP

#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#else
#include <fstream>
#include <sstream>
#include <unistd.h>
#endif

class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();
    
    struct MemoryInfo {
        unsigned long totalMemoryKB;
        unsigned long usedMemoryKB;
	unsigned long freeMemoryKB;
        float memoryUsagePercent;
	double totalGB() const { return totalMemoryKB / (1024*1024) };
	double usedGB() const { return usedMemoryKB / (1024 * 1024) };
    };
    
    struct CpuInfo {
        float cpuUsagePercent;
        int coreCount;
    };
    
    MemoryInfo getMemoryInfo() const;
    CpuInfo getCpuInfo() const;
    std::string getHostname() const;
    std::string getIpAddress() const;
    std::string getUptime() const;
    void start(chrono::milliseconds interval = chrono::milliseconds(1000));
    void stop();
    
private:

    MemoryInfo sampleMemory() const;
    CpuInfo sampleCpu();

    mutable std::mutex m_mutex;
    MemoryInfo m_memInfo{};
    CpuInfo m_cpuInfo{};

   std::atomi<bool> m_running{false}l
   std::thread m_thread;
   std::chrono::milliseconds m_interval(1000);

#ifdef _WIN32
   ULARGE_INTEGER m_prevIdleTime{};
   ULARGE_INTEGER m_prevKernelTime{};
   ULARGE_INTEGER m_prevUserTime{};
#else
   unsigned long long m_prevIdle{0};
   unsigned long long m_prevTotal{0};
#endif

   void pollLoop();
};

#endif 

