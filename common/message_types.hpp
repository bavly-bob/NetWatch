#pragma once
// message_types.hpp
// Plain-C++ data structs shared by both the Agent and the Dashboard.
// No Qt headers here — this file is compiled by both targets.

#include <string>
#include <vector>

struct ProcessInfo {
    int         pid        = 0;
    std::string name;
    double      cpu_usage  = 0.0;
    double      mem_usage  = 0.0;  // MB
};

struct SystemStats {
    std::string hostname;
    std::string ip_address;
    double      cpu_total    = 0.0;
    double      ram_used_gb  = 0.0;
    double      ram_total_gb = 0.0;
    std::string uptime;
    std::vector<ProcessInfo> processes;
};
