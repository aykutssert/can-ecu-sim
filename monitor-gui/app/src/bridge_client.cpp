#include "bridge_client.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <chrono>

namespace monitor {

namespace {

double now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

// Parses a "ID#HEXDATA" line (matching canbus::format_frame's output).
bool parse_line(const std::string& line, uint32_t& id, std::array<uint8_t, 8>& data,
                 uint8_t& dlc) {
    auto hash = line.find('#');
    if (hash == std::string::npos) {
        return false;
    }

    std::string id_str = line.substr(0, hash);
    std::string data_str = line.substr(hash + 1);
    while (!data_str.empty() &&
           std::isxdigit(static_cast<unsigned char>(data_str.back())) == 0) {
        data_str.pop_back();
    }

    if (id_str.empty() || id_str.size() > 8 || data_str.size() % 2 != 0 ||
        data_str.size() > 16) {
        return false;
    }

    try {
        id = static_cast<uint32_t>(std::stoul(id_str, nullptr, 16));
        dlc = static_cast<uint8_t>(data_str.size() / 2);
        for (uint8_t i = 0; i < dlc; ++i) {
            data[i] = static_cast<uint8_t>(std::stoul(data_str.substr(i * 2, 2), nullptr, 16));
        }
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

} // namespace

BridgeClient::BridgeClient(std::string host, uint16_t port)
    : host_(std::move(host)), port_(port), thread_(&BridgeClient::run, this) {}

BridgeClient::~BridgeClient() {
    stop_.store(true);
    if (thread_.joinable()) {
        thread_.join();
    }
}

std::map<uint32_t, FrameInfo> BridgeClient::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frames_;
}

void BridgeClient::run() {
    while (!stop_.load()) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(fd);
            connected_.store(false);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        connected_.store(true);
        std::string buffer;
        char chunk[256];

        while (!stop_.load()) {
            ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
            if (n > 0) {
                buffer.append(chunk, static_cast<size_t>(n));
                size_t pos;
                while ((pos = buffer.find('\n')) != std::string::npos) {
                    std::string line = buffer.substr(0, pos);
                    buffer.erase(0, pos + 1);

                    uint32_t id = 0;
                    std::array<uint8_t, 8> data{};
                    uint8_t dlc = 0;
                    if (parse_line(line, id, data, dlc)) {
                        std::lock_guard<std::mutex> lock(mutex_);
                        FrameInfo& info = frames_[id];
                        info.data = data;
                        info.dlc = dlc;
                        if (info.count == 0) {
                            info.first_seen = now_seconds();
                        }
                        info.count++;
                        info.last_seen = now_seconds();
                    }
                }
            } else if (n == 0) {
                break;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                break;
            }
        }

        close(fd);
        connected_.store(false);
    }
}

} // namespace monitor
