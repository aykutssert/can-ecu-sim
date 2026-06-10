#pragma once

#include <linux/can.h>

#include <string>

namespace canbus {

// RAII wrapper around a SocketCAN raw (CAN_RAW) socket bound to one
// interface (e.g. "vcan0"). Not copyable: each instance owns one fd.
class CanSocket {
public:
    // Throws std::system_error if the socket cannot be created, the
    // interface does not exist, or the bind fails.
    explicit CanSocket(const std::string& interface);
    ~CanSocket();

    CanSocket(const CanSocket&) = delete;
    CanSocket& operator=(const CanSocket&) = delete;

    // Throws std::system_error on write failure.
    void send(const can_frame& frame) const;

    // Blocks until a frame arrives or the receive timeout elapses.
    // Returns false on timeout, throws std::system_error on other errors.
    bool receive(can_frame& frame) const;

    // Sets SO_RCVTIMEO. 0 means block forever (the default).
    void set_receive_timeout(int milliseconds) const;

    int fd() const { return fd_; }

private:
    int fd_;
};

} // namespace canbus
