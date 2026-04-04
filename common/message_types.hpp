#ifndef MESSAGE_TYPES_HPP
#define MESSAGE_TYPES_HPP

#include <string>
#include <vector>
#include <QMetaType>

// This represents a single row in your process table
struct ProcessInfo {
    int pid;
    std::string name;
    double cpu_usage;
    double mem_usage;
};

// This represents the entire packet of data sent from the Agent to the Dashboard
struct SystemStats {
    double cpu_total;
    double ram_used_gb;
    double ram_total_gb;
    std::string uptime;
    std::string ip_address;
    std::vector<ProcessInfo> processes;
};

// This tells Qt how to handle these custom types in Signals and Slots
Q_DECLARE_METATYPE(ProcessInfo)
Q_DECLARE_METATYPE(SystemStats)

#endif // MESSAGE_TYPES_HPP
