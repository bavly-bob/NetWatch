#pragma once

namespace netwatch {

// The port the Dashboard listens on.
// Both the Agent and Dashboard must agree on this number.
constexpr unsigned short DEFAULT_PORT = 9000;

// How often the agent sends a stats update (milliseconds)
constexpr int POLL_INTERVAL_MS = 2000;

} // namespace netwatch
