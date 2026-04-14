#include "../include/process_monitor.hpp"

#include <algorithm>
#include <cstring>

#ifdef _WIN32
   #include <tlhelp32.h>
   #include <psapi.h>
   #pragma comment (lib, "psapi.lib")
#else
   #include <dirent.h>
   #include <fstream>
   #include <sstream>
   #include <ctime>
   #include <sys/types.h>
   #include <unistd.h>
#endif


ProcessMonitor::ProcessMonitor() = default;

ProcessMonitor::~ProcessMonitor() {
    if (m_running) stop();
}

std::vector<ProcessMonitor::ProcessInfo> ProcessMonitor::getProcessList() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_processes;
}

ProcessMonitor::ProcessInfo ProcessMonitor::getProcessInfo(uint32_t pid) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (const auto& p : m_processes)
	    if(p.pid == pid) return p;
	return ProcessInfo{pid, "", 0.0f, 0};
}

void ProcessMonitor::start(std::chrono::milliseconds interval) {
     if(m_running) return;
     m_interval = interval;
     m_running = true;
     m_thread = std::thread(&ProcessMonitor::pollLoop, this);
}

void ProcessMonitor::stop() {
    m_running = false;
    if(m_thread.joinable()) m_thread.join();
}

void ProcessMonitor::pollLoop()
{
	while(m_running) {
	    auto list = sampleProcesses();
	    {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_processes = std::move(list);
	    }
	    std::this_thread::sleep_for(m_interval);
	}
}

#ifdef _WIN32

std::vector<ProcessMonitor::ProcessInfo> ProcessMonitor::sampleProcesses() {
	std::vector<ProcessInfo> result;

	FILETIME nowFt{};
	GetSystemTimeAsFileTime(&nowFt);
	ULONGLONG now = (static_cast<ULONGLONG>(nowFt.dwHighDateTime) << 32) | nowFt.dwLowDateTime;

	SYSTEM_INFO si{};
	GetSystemInfo(&si);
	int numCores = static_cast<int>(si.dwNumberOfProcessors);
	if (numCores < 1) numCores = 1;

	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(snap == INVALID_HANDLE_VALUE) return result;

	PROCESSENTRY32W entry{};
	entry.dwSize = sizeof(entry);

	if(!Process32FirstW(snap, &entry)) {
	   CloseHandle(snap);
	   return result;
	}

	do {
<<<<<<< HEAD
		uint32_t pid = entry.th32ProcessID;
		if(pid == 0) continue;

		HANDLE hProc = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if(!hProc) continue;

		ProcessInfo info{};
		info.pid = pid;

		char narrow[MAX_PATH] = {};
		WideCharToMultiByte(CP_UTF8, 0, entry.szExeFile, -1, narrow, sizeof(narrow), nullptr, nullptr);
		

		PROCESS_MEMORY_COUNTERS pmc{};
		pmc.cb = sizeof(pmc);
		if(GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc)))
			info.memoryUsageKB = static_cast<uint64_t>(pmc.WorkingSetSize / 1024);
		
		FILETIME creation, exit, kernel, user;
		if(GetProcessTimes(hProc, &creation, &exit, &kernel, &user)) {
			ULONGLONG kTime = static_cast<ULONGLONG>(kernel.dwHighDateTime << 32) | kernel.dwLowDateTime;
			ULONGLONG uTime = static_cast<ULONGLONG>(user.dwHighDateTime << 32) | user.dwLowDateTime;

			auto it = m_prevTimes.find(pid);
			if(it != m_prevTimes.end()) {
				ULONGLONG deltaK = kTime - it->second.kernel;
				ULONGLONG deltaU = uTime - it->second.user;
				ULONGLONG deltaWall = (now > it->second.wallClock) ? (now - it->second.wallClock) : 1;

				float usage = 100.0f * static_cast<float>(deltaK + deltaU) / static_cast<float>(deltaWall * numCores);
				info.cpuUsagePercent = std::max(0.0f, std::min(100.0f, usage));
			}

			m_prevTimes[pid] = {kTime, uTime, now, numCores};
		}

		CloseHandle(hProc);
		result.push_back(std::move(info));

	} while (Process32NextW(snap, &entry));

	CloseHandle(snap);

	std::sort(result.begin(), result.end(), [] (const ProcessInfo& a, const ProcessInfo& b) {
		return a.cpuUsagePercent > b.cpuUsagePercent;
	});

	return result;

=======
		ProcessInfo info{};
		info.pid = entry.th32ProcessID;
		char buf[MAX_PATH] = {};
		WideCharToMultiByte(CP_UTF8, 0, entry.szExeFile, -1, buf, MAX_PATH, nullptr, nullptr);
		info.name = buf;
		info.cpuUsagePercent = 0.0f;
		info.memoryUsageKB = 0;
		result.push_back(info);
	} while(Process32NextW(snap, &entry));

	CloseHandle(snap);
	return result;
>>>>>>> 8584652780bfd5e594957383d668548e0d51a06f
}

#else

<<<<<<< HEAD
namespace {


bool readProcStatus(uint32_t pid, const std::string& key, unsigned long long& out)
{
    std::ifstream f("/proc/" + std::to_string(pid) + "/status");
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind(key, 0) == 0) {
            std::istringstream ss(line.substr(key.size()));
            ss >> out;
            return true;
        }
    }
    return false;
}


bool readProcStat(uint32_t pid,
                  std::string& name,
                  unsigned long long& utime,
                  unsigned long long& stime)
{
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    if (!f.is_open()) return false;

    std::string line;
    if (!std::getline(f, line)) return false;
    
    auto nameStart = line.find('(');
    auto nameEnd   = line.rfind(')');
    if (nameStart == std::string::npos || nameEnd == std::string::npos)
        return false;

    name = line.substr(nameStart + 1, nameEnd - nameStart - 1);

    std::istringstream ss(line.substr(nameEnd + 2));
    char   state;
    int    ppid, pgrp, session, tty, tpgid;
    unsigned long flags;
    unsigned long long minflt, cminflt, majflt, cmajflt;
    ss >> state >> ppid >> pgrp >> session >> tty >> tpgid >> flags
       >> minflt >> cminflt >> majflt >> cmajflt
       >> utime >> stime;

    return true;
}


unsigned long long monotonicJiffies(long clkTck)
{
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<unsigned long long>(ts.tv_sec)  * clkTck +
           static_cast<unsigned long long>(ts.tv_nsec) * clkTck / 1'000'000'000ULL;
}

} 

std::vector<ProcessMonitor::ProcessInfo> ProcessMonitor::sampleProcesses()
{
    std::vector<ProcessInfo> result;

    long clkTck = sysconf(_SC_CLK_TCK);
    if (clkTck <= 0) clkTck = 100;

    unsigned long long wallNow = monotonicJiffies(clkTck);

    DIR* dir = opendir("/proc");
    if (!dir) return result;

    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN)
            continue;

        uint32_t pid = 0;
        bool isNum = true;
        for (const char* c = entry->d_name; *c; ++c) {
            if (*c < '0' || *c > '9') { isNum = false; break; }
        }
        if (!isNum) continue;
        pid = static_cast<uint32_t>(std::stoul(entry->d_name));

        std::string name;
        unsigned long long utime = 0, stime = 0;
        if (!readProcStat(pid, name, utime, stime)) continue;

        ProcessInfo info{};
        info.pid  = pid;
        info.name = name;

        unsigned long long rssKB = 0;
        if (readProcStatus(pid, "VmRSS:", rssKB))
            info.memoryUsageKB = rssKB;

        unsigned long long cpuTime = utime + stime;
        auto it = m_prevTimes.find(pid);
        if (it != m_prevTimes.end()) {
            unsigned long long deltaCpu  = cpuTime        - (it->second.utime + it->second.stime);
            unsigned long long deltaWall = (wallNow > it->second.wallClock)
                                         ? (wallNow - it->second.wallClock) : 1;
            float usage = 100.0f * static_cast<float>(deltaCpu) /
                                   static_cast<float>(deltaWall);
            info.cpuUsagePercent = std::max(0.0f, std::min(100.0f, usage));
        }

        m_prevTimes[pid] = {utime, stime, wallNow, clkTck};
        result.push_back(std::move(info));
    }
    closedir(dir);

    std::sort(result.begin(), result.end(),
        [](const ProcessInfo& a, const ProcessInfo& b){
            return a.cpuUsagePercent > b.cpuUsagePercent;
        });

    return result;
=======
std::vector<ProcessMonitor::ProcessInfo> ProcessMonitor::sampleProcesses() {
	std::vector<ProcessInfo> result;
	// TODO: Write Non-Windows logic
	return result;
>>>>>>> 8584652780bfd5e594957383d668548e0d51a06f
}

#endif
