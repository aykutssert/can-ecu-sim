#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace monitor {

// Latest known state for one CAN ID.
struct FrameInfo {
    uint8_t dlc = 0;
    std::array<uint8_t, 8> data{};
    uint64_t count = 0;
    double first_seen = 0.0;
    double last_seen = 0.0;
};

// Connects to a can_monitor_bridge TCP stream ("ID#HEXDATA" lines, one per
// frame) and maintains a snapshot of the latest state per CAN ID. Runs a
// background thread that reconnects automatically if the bridge is not
// reachable yet or drops the connection.
class BridgeClient {
public:
    BridgeClient(std::string host, uint16_t port);
    ~BridgeClient();

    BridgeClient(const BridgeClient&) = delete;
    BridgeClient& operator=(const BridgeClient&) = delete;

    // Snapshot of the current per-ID state, for rendering.
    std::map<uint32_t, FrameInfo> snapshot() const;

    bool connected() const { return connected_.load(); }

private:
    void run();

    std::string host_;
    uint16_t port_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> connected_{false};
    mutable std::mutex mutex_;
    std::map<uint32_t, FrameInfo> frames_;
    std::thread thread_;
};

} // namespace monitor
