#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <csignal>

#include "system_monitor.hpp"
#include "process_monitor.hpp"

// Networking
#include "client.hpp"

// Shared types
#include "message_types.hpp"
#include "json_serializer.hpp"
#include "config.hpp"

// ─── Graceful shutdown on Ctrl-C ─────────────────────────────────────────────
static std::atomic<bool> g_running{true};
static void onSignal(int) { g_running = false; }

// ─── Build a SystemStats snapshot from the two monitors ──────────────────────
static SystemStats buildStats(SystemMonitor& sys, ProcessMonitor& proc) {
    SystemStats stats;
    stats.hostname     = sys.getHostname();
    stats.ip_address   = sys.getIpAddress();
    stats.uptime       = sys.getUptime();

    auto mem = sys.getMemoryInfo();
    auto cpu = sys.getCpuInfo();
    stats.cpu_total    = static_cast<double>(cpu.cpuUsagePercent);
    stats.ram_used_gb  = mem.usedGB();
    stats.ram_total_gb = mem.totalGB();

    // Convert ProcessMonitor::ProcessInfo → common ProcessInfo
    for (const auto& p : proc.getProcessList()) {
        ProcessInfo pi;
        pi.pid       = static_cast<int>(p.pid);
        pi.name      = p.name;
        pi.cpu_usage = static_cast<double>(p.cpuUsagePercent);
        pi.mem_usage = static_cast<double>(p.memoryUsageKB) / 1024.0; // KB → MB
        stats.processes.push_back(pi);
    }
    return stats;
}

// ─── Entry point ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // Accept optional host argument: ./NetWatchAgent 192.168.1.5
    std::string host = "127.0.0.1";
    if (argc >= 2) host = argv[1];

    uint16_t port = netwatch::DEFAULT_PORT;

    std::cout << "[Agent] NetWatch Agent starting...\n"
              << "[Agent] Will connect to Dashboard at " << host << ":" << port << "\n";

    // ── Install signal handler ───────────────────────────────────────────────
    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    // ── Start monitors ───────────────────────────────────────────────────────
    SystemMonitor sysMonitor;
    ProcessMonitor procMonitor;
    sysMonitor.start(std::chrono::milliseconds(netwatch::POLL_INTERVAL_MS));
    procMonitor.start(std::chrono::milliseconds(netwatch::POLL_INTERVAL_MS));

    // ── Set up networking ────────────────────────────────────────────────────
    boost::asio::io_context io;

    // Keep io_context alive even when there is no pending work
    auto work = boost::asio::make_work_guard(io);

    auto client = std::make_shared<netwatch::networking::Client>(io);
    client->connect(host, port);

    // Run io_context on a background thread so send() is non-blocking
    std::thread ioThread([&io] { io.run(); });

    // ── Sending loop ─────────────────────────────────────────────────────────
    std::cout << "[Agent] Running. Press Ctrl+C to stop.\n";

    while (g_running) {
        SystemStats stats = buildStats(sysMonitor, procMonitor);

        // Serialize and send raw payload.
        // Connection::send() already applies the [4-byte length][payload] framing.
        std::string json = netwatch::serialize(stats);
        client->send(json);

        std::cout << "[Agent] Sent: CPU=" << stats.cpu_total
                  << "% RAM=" << stats.ram_used_gb << "/" << stats.ram_total_gb << " GB"
                  << " Processes=" << stats.processes.size() << "\n";

        std::this_thread::sleep_for(
            std::chrono::milliseconds(netwatch::POLL_INTERVAL_MS));
    }

    // ── Clean shutdown ───────────────────────────────────────────────────────
    std::cout << "[Agent] Shutting down...\n";
    sysMonitor.stop();
    procMonitor.stop();
    client->disconnect();
    work.reset();           // allow io.run() to return
    io.stop();
    if (ioThread.joinable()) ioThread.join();

    std::cout << "[Agent] Done.\n";
    return 0;
}
