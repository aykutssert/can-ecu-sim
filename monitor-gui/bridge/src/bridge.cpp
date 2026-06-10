// Bridges a SocketCAN interface to a TCP stream of formatted CAN frames
// (one "ID#HEXDATA" line per frame), so the macOS-native monitor GUI can
// follow live bus traffic through Lima's automatic port forwarding.
#include "canbus/can_socket.hpp"
#include "canbus/frame.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <system_error>

namespace {

[[noreturn]] void throw_errno(const std::string& what) {
    throw std::system_error(errno, std::generic_category(), what);
}

int make_listen_socket(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw_errno("socket");
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        throw_errno("bind");
    }
    if (listen(fd, 1) < 0) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        throw_errno("listen");
    }
    return fd;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: can_monitor_bridge <interface> <port>\n";
        return 1;
    }

    try {
        canbus::CanSocket can(argv[1]);
        int listen_fd = make_listen_socket(static_cast<uint16_t>(std::stoi(argv[2])));

        std::cout << "listening on port " << argv[2] << ", forwarding " << argv[1] << "\n";

        while (true) {
            int client_fd = accept(listen_fd, nullptr, nullptr);
            if (client_fd < 0) {
                throw_errno("accept");
            }
            std::cout << "client connected\n";

            can_frame frame{};
            while (can.receive(frame)) {
                std::string line = canbus::format_frame(frame) + "\n";
                if (send(client_fd, line.data(), line.size(), MSG_NOSIGNAL) < 0) {
                    break;
                }
            }
            close(client_fd);
            std::cout << "client disconnected\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "can_monitor_bridge: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
