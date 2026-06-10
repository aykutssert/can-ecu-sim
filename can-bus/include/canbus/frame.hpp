#pragma once

#include <linux/can.h>

#include <cstdint>
#include <string>
#include <vector>

namespace canbus {

// Builds a can_frame from a CAN ID and up to CAN_MAX_DLEN (8) data bytes.
// Throws std::invalid_argument if data has more than 8 bytes.
can_frame make_frame(canid_t id, const std::vector<uint8_t>& data);

// Formats a frame as "ID#HEXDATA", e.g. "123#DEADBEEF". ID is printed in hex
// without leading zeros, matching can-utils conventions.
std::string format_frame(const can_frame& frame);

// Parses "ID#HEXDATA" (hex CAN ID, 0-16 hex digits of data, even length).
// Throws std::invalid_argument on malformed input.
can_frame parse_frame(const std::string& text);

} // namespace canbus
