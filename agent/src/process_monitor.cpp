#include "process_monitor.hpp"

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
}

#else

std::vector<ProcessMonitor::ProcessInfo> ProcessMonitor::sampleProcesses() {
	std::vector<ProcessInfo> result;
	// TODO: Write Non-Windows logic
	return result;
}

#endif
