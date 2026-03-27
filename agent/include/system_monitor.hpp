#ifndef SYSTEM_MONITOR_HPP
#define SYSTEM_MONITOR_HPP

#include <string>
#include <vector>

class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();
    
    struct MemoryInfo {
        unsigned long totalMemory;
        unsigned long usedMemory;
        float memoryUsagePercent;
    };
    
    struct CpuInfo {
        float cpuUsagePercent;
        int coreCount;
    };
    
    MemoryInfo getMemoryInfo() const;
    CpuInfo getCpuInfo() const;
    void start();
    void stop();
    
private:
    bool running = false;
};

#endif // SYSTEM_MONITOR_HPP
