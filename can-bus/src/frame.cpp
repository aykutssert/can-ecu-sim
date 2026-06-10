#include "canbus/frame.hpp"

#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace canbus {

can_frame make_frame(canid_t id, const std::vector<uint8_t>& data) {
    if (data.size() > CAN_MAX_DLEN) {
        throw std::invalid_argument("frame data exceeds 8 bytes");
    }
    can_frame frame{};
    frame.can_id = id;
    frame.len = static_cast<uint8_t>(data.size());
    std::memcpy(frame.data, data.data(), data.size());
    return frame;
}

std::string format_frame(const can_frame& frame) {
    char id_buf[16];
    std::snprintf(id_buf, sizeof(id_buf), "%X", frame.can_id & CAN_EFF_MASK);

    std::string out = id_buf;
    out += '#';
    char byte_buf[3];
    for (int i = 0; i < frame.len; ++i) {
        std::snprintf(byte_buf, sizeof(byte_buf), "%02X", frame.data[i]);
        out += byte_buf;
    }
    return out;
}

can_frame parse_frame(const std::string& text) {
    auto sep = text.find('#');
    if (sep == std::string::npos || sep == 0) {
        throw std::invalid_argument("expected format ID#HEXDATA, got: " + text);
    }

    std::string id_part = text.substr(0, sep);
    std::string data_part = text.substr(sep + 1);

    if (data_part.size() % 2 != 0 || data_part.size() > 2 * CAN_MAX_DLEN) {
        throw std::invalid_argument("data must be 0-16 hex digits (even length): " + text);
    }

    canid_t id;
    try {
        id = static_cast<canid_t>(std::stoul(id_part, nullptr, 16));
    } catch (const std::exception&) {
        throw std::invalid_argument("invalid hex CAN ID: " + id_part);
    }

    std::vector<uint8_t> data;
    data.reserve(data_part.size() / 2);
    for (size_t i = 0; i < data_part.size(); i += 2) {
        try {
            data.push_back(static_cast<uint8_t>(std::stoul(data_part.substr(i, 2), nullptr, 16)));
        } catch (const std::exception&) {
            throw std::invalid_argument("invalid hex byte in data: " + data_part.substr(i, 2));
        }
    }

    return make_frame(id, data);
}

} // namespace canbus
