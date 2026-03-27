#include <iostream>
#include "system_monitor.hpp"
#include "process_monitor.hpp"

int main(int argc, char* argv[]) {
    std::cout << "NetWatch Agent starting..." << std::endl;
    
    SystemMonitor sysMonitor;
    ProcessMonitor procMonitor;
    
    sysMonitor.start();
    procMonitor.start();
    
    std::cout << "Agent is running. Press Ctrl+C to stop." << std::endl;
    
    // Simple loop - in real implementation would have proper event handling
    while (true) {
        auto memInfo = sysMonitor.getMemoryInfo();
        auto cpuInfo = sysMonitor.getCpuInfo();
        
        std::cout << "Memory: " << memInfo.memoryUsagePercent << "%, "
                  << "CPU Cores: " << cpuInfo.coreCount << std::endl;
        
        // Sleep for a bit (simplified - use std::chrono in production)
        for (int i = 0; i < 1000000000; ++i) {}
    }
    
    sysMonitor.stop();
    procMonitor.stop();
    
    return 0;
}
