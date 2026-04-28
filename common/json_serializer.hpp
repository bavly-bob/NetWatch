#pragma once
// json_serializer.hpp
// Hand-rolled JSON serializer/deserializer for SystemStats and ProcessInfo.
// No third-party JSON library required.

#include "message_types.hpp"
#include <string>
#include <sstream>
#include <stdexcept>

namespace netwatch {

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────

namespace detail {

// Escape a string value to be valid JSON
inline std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out += '"';
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    out += '"';
    return out;
}

// Read the next JSON string value after the given key (e.g. "key":"value")
inline std::string extractString(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return {};
    pos += pattern.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return {};
    return json.substr(pos, end - pos);
}

// Read the next JSON number value after the given key (e.g. "key":123.4)
inline double extractNumber(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return 0.0;
    pos += pattern.size();
    return std::stod(json.substr(pos));
}

} // namespace detail

// ─────────────────────────────────────────────
//  Serialize SystemStats → JSON string
// ─────────────────────────────────────────────

inline std::string serialize(const SystemStats& s) {
    std::ostringstream o;
    o << "{"
      << "\"type\":\"SystemStats\","
      << "\"hostname\":"   << detail::jsonEscape(s.hostname)   << ","
      << "\"ip_address\":" << detail::jsonEscape(s.ip_address) << ","
      << "\"cpu_total\":"  << s.cpu_total   << ","
      << "\"ram_used_gb\":" << s.ram_used_gb << ","
      << "\"ram_total_gb\":" << s.ram_total_gb << ","
      << "\"uptime\":"     << detail::jsonEscape(s.uptime) << ","
      << "\"processes\":[";

    for (std::size_t i = 0; i < s.processes.size(); ++i) {
        const auto& p = s.processes[i];
        if (i > 0) o << ",";
        o << "{"
          << "\"pid\":"   << p.pid   << ","
          << "\"name\":"  << detail::jsonEscape(p.name) << ","
          << "\"cpu\":"   << p.cpu_usage << ","
          << "\"mem\":"   << p.mem_usage
          << "}";
    }

    o << "]}";
    return o.str();
}

// ─────────────────────────────────────────────
//  Deserialize JSON string → SystemStats
// ─────────────────────────────────────────────

inline SystemStats deserialize(const std::string& json) {
    SystemStats s;
    s.hostname    = detail::extractString(json, "hostname");
    s.ip_address  = detail::extractString(json, "ip_address");
    s.cpu_total   = detail::extractNumber(json, "cpu_total");
    s.ram_used_gb = detail::extractNumber(json, "ram_used_gb");
    s.ram_total_gb= detail::extractNumber(json, "ram_total_gb");
    s.uptime      = detail::extractString(json, "uptime");

    // Parse process list
    auto arrStart = json.find("\"processes\":[");
    if (arrStart != std::string::npos) {
        arrStart = json.find('[', arrStart);
        auto arrEnd = json.find(']', arrStart);
        std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);

        // Iterate each { ... } object in the array
        std::size_t pos = 0;
        while (pos < arr.size()) {
            auto ob = arr.find('{', pos);
            if (ob == std::string::npos) break;
            auto cb = arr.find('}', ob);
            if (cb == std::string::npos) break;
            std::string obj = arr.substr(ob, cb - ob + 1);

            ProcessInfo p;
            p.pid       = static_cast<int>(detail::extractNumber(obj, "pid"));
            p.name      = detail::extractString(obj, "name");
            p.cpu_usage = detail::extractNumber(obj, "cpu");
            p.mem_usage = detail::extractNumber(obj, "mem");
            s.processes.push_back(p);

            pos = cb + 1;
        }
    }
    return s;
}

} // namespace netwatch
