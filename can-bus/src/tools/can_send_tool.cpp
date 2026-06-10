#include "canbus/can_socket.hpp"
#include "canbus/frame.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: can_send <interface> <id>#<hexdata>\n"
                     "example: can_send vcan0 123#DEADBEEF\n";
        return 1;
    }

    try {
        canbus::CanSocket sock(argv[1]);
        can_frame frame = canbus::parse_frame(argv[2]);
        sock.send(frame);
        std::cout << argv[1] << "  " << canbus::format_frame(frame) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "can_send: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
