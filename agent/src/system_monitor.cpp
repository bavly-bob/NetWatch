#include "system_monitor.hpp"

SystemMonitor::SystemMonitor() {
}

SystemMonitor::~SystemMonitor() {
    if (running) {
        stop();
    }
}

SystemMonitor::MemoryInfo SystemMonitor::getMemoryInfo() const {
    return {0, 0, 0.0f};
}

SystemMonitor::CpuInfo SystemMonitor::getCpuInfo() const {
    return {0.0f, 0};
}

void SystemMonitor::start() {
    running = true;
}

void SystemMonitor::stop() {
    running = false;
}
