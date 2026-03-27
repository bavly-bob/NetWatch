#include "process_monitor.hpp"

ProcessMonitor::ProcessMonitor() {
}

ProcessMonitor::~ProcessMonitor() {
    if (running) {
        stop();
    }
}

std::vector<ProcessMonitor::ProcessInfo> ProcessMonitor::getProcessList() const {
    return {};
}

ProcessMonitor::ProcessInfo ProcessMonitor::getProcessInfo(uint32_t pid) const {
    return {pid, "", 0.0f, 0};
}

void ProcessMonitor::start() {
    running = true;
}

void ProcessMonitor::stop() {
    running = false;
}
