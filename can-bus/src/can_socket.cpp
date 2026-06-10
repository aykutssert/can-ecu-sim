#include "canbus/can_socket.hpp"

#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <system_error>

namespace canbus {

namespace {

[[noreturn]] void throw_errno(const std::string& what) {
    throw std::system_error(errno, std::generic_category(), what);
}

} // namespace

CanSocket::CanSocket(const std::string& interface) {
    fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd_ < 0) {
        throw_errno("socket(PF_CAN)");
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    if (ioctl(fd_, SIOCGIFINDEX, &ifr) < 0) {
        int saved_errno = errno;
        close(fd_);
        errno = saved_errno;
        throw_errno("ioctl(SIOCGIFINDEX) for interface '" + interface + "'");
    }

    sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        int saved_errno = errno;
        close(fd_);
        errno = saved_errno;
        throw_errno("bind() to interface '" + interface + "'");
    }
}

CanSocket::~CanSocket() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

void CanSocket::send(const can_frame& frame) const {
    ssize_t written = ::write(fd_, &frame, sizeof(frame));
    if (written < 0) {
        throw_errno("write(can_frame)");
    }
    if (written != sizeof(frame)) {
        throw std::system_error(EIO, std::generic_category(),
                                 "write(can_frame): short write");
    }
}

bool CanSocket::receive(can_frame& frame) const {
    ssize_t n = ::read(fd_, &frame, sizeof(frame));
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;
        }
        throw_errno("read(can_frame)");
    }
    if (n != sizeof(frame)) {
        throw std::system_error(EIO, std::generic_category(),
                                 "read(can_frame): short read");
    }
    return true;
}

void CanSocket::set_receive_timeout(int milliseconds) const {
    timeval tv{};
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    if (setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        throw_errno("setsockopt(SO_RCVTIMEO)");
    }
}

} // namespace canbus
