#pragma once
// json_serializer.hpp
// Hand-rolled JSON serializer/deserializer for SystemStats and ProcessInfo.
// No third-party JSON library required.

#include "message_types.hpp"
#include <string>
#include <sstream>
#include <stdexcept>

namespace netwatch {

//  Helpers

namespace detail {

inline std::size_t skipWs(const std::string& s, std::size_t pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\n' || s[pos] == '\r' || s[pos] == '\t')) {
        ++pos;
    }
    return pos;
}

// Handle accidental double-framing: [4-byte length][json payload]
inline std::string stripOptionalFrame(const std::string& payload) {
    if (payload.empty()) return payload;
    if (payload.front() == '{') return payload;
    if (payload.size() < 5) return payload;

    const auto b0 = static_cast<unsigned char>(payload[0]);
    const auto b1 = static_cast<unsigned char>(payload[1]);
    const auto b2 = static_cast<unsigned char>(payload[2]);
    const auto b3 = static_cast<unsigned char>(payload[3]);
    const std::size_t len = (static_cast<std::size_t>(b0) << 24) |
                            (static_cast<std::size_t>(b1) << 16) |
                            (static_cast<std::size_t>(b2) << 8)  |
                            static_cast<std::size_t>(b3);
    if (len == payload.size() - 4) {
        return payload.substr(4);
    }
    return payload;
}

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
    std::string pattern = "\"" + key + "\":";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return {};
    pos += pattern.size();
    pos = skipWs(json, pos);
    if (pos >= json.size() || json[pos] != '"') return {};
    ++pos;

    std::string out;
    bool escaped = false;
    for (; pos < json.size(); ++pos) {
        const char c = json[pos];
        if (escaped) {
            switch (c) {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;
                default:   out += c;    break;
            }
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '"') {
            return out;
        }
        out += c;
    }
    return {};
}

// Read the next JSON number value after the given key (e.g. "key":123.4)
inline double extractNumber(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return 0.0;
    pos += pattern.size();
    pos = skipWs(json, pos);
    auto end = pos;
    while (end < json.size()) {
        const char c = json[end];
        const bool isNumChar = (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E';
        if (!isNumChar) break;
        ++end;
    }
    if (end <= pos) return 0.0;
    return std::stod(json.substr(pos, end - pos));
}

inline std::size_t findMatching(const std::string& s, std::size_t openPos, char openCh, char closeCh) {
    if (openPos >= s.size() || s[openPos] != openCh) return std::string::npos;
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = openPos; i < s.size(); ++i) {
        const char c = s[i];
        if (inString) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (c == '\\') {
                escaped = true;
                continue;
            }
            if (c == '"') inString = false;
            continue;
        }
        if (c == '"') {
            inString = true;
            continue;
        }
        if (c == openCh) ++depth;
        else if (c == closeCh) {
            --depth;
            if (depth == 0) return i;
        }
    }
    return std::string::npos;
}

} // namespace detail

//  Serialize SystemStats → JSON string

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

inline SystemStats deserialize(const std::string& payload) {
    const std::string json = detail::stripOptionalFrame(payload);
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
        auto arrEnd = detail::findMatching(json, arrStart, '[', ']');
        if (arrEnd == std::string::npos || arrEnd <= arrStart) return s;
        std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);

        // Iterate each { ... } object in the array
        std::size_t pos = 0;
        while (pos < arr.size()) {
            auto ob = arr.find('{', pos);
            if (ob == std::string::npos) break;
            auto cb = detail::findMatching(arr, ob, '{', '}');
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
