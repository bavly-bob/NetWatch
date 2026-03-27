#ifndef PROCESS_MONITOR_HPP
#define PROCESS_MONITOR_HPP

#include <string>
#include <vector>
#include <cstdint>

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();
    
    struct ProcessInfo {
        uint32_t pid;
        std::string name;
        float cpuUsage;
        uint64_t memoryUsage;
    };
    
    std::vector<ProcessInfo> getProcessList() const;
    ProcessInfo getProcessInfo(uint32_t pid) const;
    void start();
    void stop();
    
private:
    bool running = false;
};

#endif // PROCESS_MONITOR_HPP
