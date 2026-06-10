// Integration test: requires a vcan interface (default "vcan0", override
// with CANBUS_TEST_IFACE). Sends N frames on one socket, receives them on
// another bound to the same interface, and checks order + content.
#include "canbus/can_socket.hpp"
#include "canbus/frame.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

int main() {
    const char* iface = std::getenv("CANBUS_TEST_IFACE");
    if (!iface) {
        iface = "vcan0";
    }

    canbus::CanSocket tx(iface);
    canbus::CanSocket rx(iface);
    rx.set_receive_timeout(1000);

    std::vector<can_frame> sent;
    constexpr int kFrameCount = 10;
    for (int i = 0; i < kFrameCount; ++i) {
        can_frame frame = canbus::make_frame(
            0x100 + i, {static_cast<uint8_t>(i), 0xAA, 0xBB});
        sent.push_back(frame);
        tx.send(frame);
    }

    for (size_t i = 0; i < sent.size(); ++i) {
        can_frame received{};
        if (!rx.receive(received)) {
            std::cerr << "FAIL: timed out waiting for frame " << i << "\n";
            return 1;
        }
        const can_frame& expected = sent[i];
        bool match = received.can_id == expected.can_id &&
                      received.len == expected.len &&
                      std::memcmp(received.data, expected.data, received.len) == 0;
        if (!match) {
            std::cerr << "FAIL: frame " << i << " mismatch: got "
                      << canbus::format_frame(received) << ", want "
                      << canbus::format_frame(expected) << "\n";
            return 1;
        }
    }

    std::cout << "OK: " << sent.size() << " frames, in order, byte-exact\n";
    return 0;
}
