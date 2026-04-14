#include "../include/system_monitor.hpp"

#include <stdexcept>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
  #pragma comment(lib, "kernel32.lib")
#else
  #include <sys/sysinfo.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <ifaddrs.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <unistd.h>
#endif


SystemMonitor::SystemMonitor() {
   #ifdef _WIN32
	FILETIME idle, kernel, user;
	if (GetSystemTimes(&idle, &kernel, &user)) {
	   m_prevIdleTime.LowPart = idle.dwLowDateTime;
	   m_prevIdleTime.HighPart = idle.dwHighDateTime;
	   m_prevKernelTime.LowPart = kernel.dwLowDateTime;
	   m_prevKernelTime.HighPart = kernel.dwHighDateTime;
	   m_prevUserTime.LowPart = user.dwLowDateTime;
	   m_prevUserTime.HighPart = user.dwHighDateTime;
	}
   #else
	sampleCpu();
   #endif
}

SystemMonitor::~SystemMonitor() {
    if (m_running) {
        stop();
    }
}

SystemMonitor::MemoryInfo SystemMonitor::getMemoryInfo() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_memInfo;
}

SystemMonitor::CpuInfo SystemMonitor::getCpuInfo() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cpuInfo;
}

std::string SystemMonitor::getHostname() const {
    char buf[256] = {};
    if (gethostname(buf, sizeof(buf)) == 0)
	return std::string(buf);
    return "unknown";
}

std::string SystemMonitor::getIpAddress() const {
#ifdef _WIN32
	char hostname[256] = {};
	gethostname(hostname, sizeof(hostname));

	addrinfo hints{}, *res = nullptr;
	hints.ai_family = AF_INET;
	if(getaddrinfo(hostname, nullptr, &hints, &res) != 0 || !res)
	   return "0.0.0.0";
	char ip[INET_ADDRSTRLEN] = {};
	auto* sa = reinterpret_cast<sockaddr_in*>(res->ai_addr);
	inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));
	freeaddrinfo(res);
	return std::string(ip);
#else
	ifaddrs* ifaddr = nullptr;
	if(getifaddrs(&ifaddr) == -1) return "0.0.0.0";

	std::string result = "0.0.0.0";
	for (auto* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
	   if(!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
	   if(std::string(ifa->ifa_name) == "lo") continue;

	   char ip[INET_ADDRSTRLEN] = {};
	   auto* sa = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
	   inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));
	   result = std::string(ip);
	   break;
	}
	freeifaddrs(ifaddr);
	return result;
#endif
}

std::string SystemMonitor::getUptime() const {
	long seconds = 0;

#ifdef _WIN32
	seconds = static_cast<long>(GetTickCount() / 1000ULL);
#else
	struct sysinfo si{};
	if (sysinfo(&si) == 0)
	   seconds = si.uptime;
#endif

	long d = seconds/86400;
	long h = (seconds % 86400) / 3600;
	long m = (seconds % 3600) / 60;

	std::string out;
	if(d>0) out += std::to_string(d) + "d ";
	out += std::to_string(h) + "h " + std::to_string(m) + "m ";
	return out;
}

SystemMonitor::MemoryInfo SystemMonitor::sampleMemory() const
{
	MemoryInfo info{};

#ifdef _WIN32
	MEMORYSTATUSEX ms{};
	ms.dwLength = sizeof(ms);
	if (GlobalMemoryStatusEx(&ms)) {
	   info.totalMemoryKB = static_cast<unsigned long>(ms.ullTotalPhys / 1024ULL);
	   info.freeMemoryKB = static_cast<unsigned long>(ms.ullAvailPhys / 1024ULL);
	   if(info.totalMemoryKB > 0)
		info.memoryUsagePercent = 100.0f * static_cast<float>(info.usedMemoryKB) / static_cast<float>(info.totalMemoryKB);
	}
#else
	std::ifstream f("/proc/meminfo");
	if(!f.is_open()) return info;

	unsigned long totalKB = 0, freeKB = 0, buffersKB = 0, cachedKB = 0, reclaimableKB = 0;
	std::string key;
	unsigned long value = 0;
	std::string unit;

	while (f >> key >> value) {
	   f >> unit;
	   if(key == "MemTotal:") totalKB = value;
	   else if (key == "MemFree:") freeKB = value;
	   else if (key == "Buffers:") buffersKB = value;
	   else if (key == "Cached:") cachedKB = value;
	   else if (key == "SReclaimable:") reclaimableKB = value;
	}

	unsigned long availKB = freeKB + buffersKB + cachedKB + reclaimableKB;
	info.totalMemoryKB = totalKB;
	info.usedMemoryKB = (totalKB > availKB) ? (totalKB - availKB) : 0;
	info.freeMemoryKB = availKB;
	if (totalKB > 0)
	   info.memoryUsagePercent = 100.0f * static_cast<float>(info.usedMemoryKB)/static_cast<float>(totalKB);
#endif

	return info;
}

SystemMonitor::CpuInfo SystemMonitor::sampleCpu() {
	CpuInfo info{};

#ifdef _WIN32
	SYSTEM_INFO si{};
	GetSystemInfo(&si);
	info.coreCount = static_cast<int>(si.dwNumberOfProcessors);

	FILETIME idle, kernel, user;
	if(!GetSystemTimes(&idle, &kernel, &user)) return info;

	ULARGE_INTEGER curIdle, curKernel, curUser;
	curIdle.LowPart = idle.dwLowDateTime; curIdle.HighPart = idle.dwHighDateTime;
	curKernel.LowPart = kernel.dwLowDateTime; curKernel.HighPart = kernel.dwHighDateTime;
	curUser.LowPart = user.dwLowDateTime; curUser.HighPart = user.dwHighDateTime;

	ULONGLONG deltaIdle = curIdle.QuadPart - m_prevIdleTime.QuadPart;
	ULONGLONG deltaKernel = curKernel.QuadPart - m_prevKernelTime.QuadPart;
	ULONGLONG deltaUser = curUser.QuadPart - m_prevUserTime.QuadPart;
	ULONGLONG deltaTotal = deltaKernel + deltaUser;

	if(deltaTotal>0)
	   info.cpuUsagePercent = 100.0f * static_cast<float>(deltaTotal - deltaIdle) / static_cast<float>(deltaTotal);

	m_prevIdleTime = curIdle;
	m_prevKernelTime = curKernel;
	m_prevUserTime = curUser;

#else
	int cores = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
	info.coreCount = (cores > 0) ? cores : 1;

	std::ifstream f("/proc/stat");
	if(!f.is_open()) return info;

	std::string label;
	unsigned long long user2, nice, system, idle2, iowait, irq, softirq, steal;
	f >> label >> user2 >> nice >> system >> idle2 >> iowait >> irq >> softirq >> steal;

	unsigned long long idleTotal = idle2 + iowait;
	unsigned long long total = user2 + nice + system + idle2 + iowait + irq + softirq + steal;

	unsigned long long deltaIdle = idleTotal - m_prevIdle;
	unsigned long long deltaTotal = total - m_prevTotal;

	if(deltaTotal > 0)
	   info.cpuUsagePercent = 100.0f * static_cast<float>(deltaTotal - deltaIdle) / static_cast<float>(deltaTotal);

	m_prevIdle = idleTotal;
	m_prevTotal = total;
#endif

	info.cpuUsagePercent = std::max(0.0f, std::min(100.0f, info.cpuUsagePercent));
	return info;
}

void SystemMonitor::start(std::chrono::milliseconds interval) {
    if(m_running) return;
    m_interval = interval;
    m_running = true;
    m_thread = std::thread(&SystemMonitor::pollLoop, this);
}

void SystemMonitor::stop() {
    m_running = false;
    if(m_thread.joinable()) m_thread.join();
}

void SystemMonitor::pollLoop() {
	while(m_running) {
	   MemoryInfo mem = sampleMemory();
	   CpuInfo cpu = sampleCpu();

   	   {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_memInfo = mem;
		m_cpuInfo = cpu;
	   }

	   std::this_thread::sleep_for(m_interval);
	}
}
