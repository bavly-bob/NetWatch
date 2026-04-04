#ifndef MESSAGE_TYPES_HPP
#define MESSAGE_TYPES_HPP

#include <string>
#include <vector>
#include <QMetaType>

struct ProcessInfo {
    int pid;
    std::string name;
    double cpu_usage;
    double mem_usage;
};

struct SystemStats {
    std::string hostname;
    std::string ip_address;
    double cpu_total;
    double ram_used_gb;
    double ram_total_gb;
    std::string uptime;
    std::vector<ProcessInfo> processes;
};


Q_DECLARE_METATYPE(ProcessInfo)
Q_DECLARE_METATYPE(SystemStats)

#endif
