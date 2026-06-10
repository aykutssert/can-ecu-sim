#include "canbus/can_socket.hpp"
#include "canbus/frame.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: can_dump <interface>\n";
        return 1;
    }

    try {
        canbus::CanSocket sock(argv[1]);
        can_frame frame{};
        while (sock.receive(frame)) {
            std::cout << argv[1] << "  " << canbus::format_frame(frame) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "can_dump: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
