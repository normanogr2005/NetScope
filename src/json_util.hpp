#pragma once

#include <iomanip>
#include <sstream>
#include <string>

namespace netscope {
namespace detail {

// Return the byte length of a valid UTF-8 sequence starting at offset.
// Invalid bytes are escaped individually so the result is always valid UTF-8.
inline std::size_t valid_utf8_sequence_length(const std::string& value,
                                              std::size_t offset) {
    const auto byte = [&value](std::size_t i) {
        return static_cast<unsigned char>(value[i]);
    };
    const auto remaining = value.size() - offset;
    const unsigned char first = byte(offset);

    if (first >= 0xC2U && first <= 0xDFU) {
        if (remaining >= 2U && byte(offset + 1U) >= 0x80U &&
            byte(offset + 1U) <= 0xBFU) {
            return 2U;
        }
        return 0U;
    }

    if (first >= 0xE0U && first <= 0xEFU) {
        if (remaining < 3U) return 0U;
        const unsigned char second = byte(offset + 1U);
        const unsigned char third = byte(offset + 2U);
        const bool second_valid =
            (first == 0xE0U) ? (second >= 0xA0U && second <= 0xBFU) :
            (first == 0xEDU) ? (second >= 0x80U && second <= 0x9FU) :
                               (second >= 0x80U && second <= 0xBFU);
        return second_valid && third >= 0x80U && third <= 0xBFU ? 3U : 0U;
    }

    if (first >= 0xF0U && first <= 0xF4U) {
        if (remaining < 4U) return 0U;
        const unsigned char second = byte(offset + 1U);
        const unsigned char third = byte(offset + 2U);
        const unsigned char fourth = byte(offset + 3U);
        const bool second_valid =
            (first == 0xF0U) ? (second >= 0x90U && second <= 0xBFU) :
            (first == 0xF4U) ? (second >= 0x80U && second <= 0x8FU) :
                               (second >= 0x80U && second <= 0xBFU);
        return second_valid &&
               third >= 0x80U && third <= 0xBFU &&
               fourth >= 0x80U && fourth <= 0xBFU ? 4U : 0U;
    }

    return 0U;
}

inline void append_unicode_escape(std::string& out, unsigned char byte) {
    std::ostringstream escaped;
    escaped << "\\u" << std::hex << std::nouppercase
            << std::setfill('0') << std::setw(4)
            << static_cast<unsigned int>(byte);
    out += escaped.str();
}

} // namespace detail

// Escape a string for use inside a JSON string value. Valid UTF-8 is preserved.
// JSON control bytes and malformed UTF-8 bytes are escaped as \u00XX, ensuring
// the resulting JSON text remains valid UTF-8 and cannot break an NDJSON line.
inline std::string json_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8U);

    for (std::size_t i = 0; i < value.size();) {
        const auto ch = static_cast<unsigned char>(value[i]);
        switch (ch) {
        case '"': out += "\\\""; ++i; break;
        case '\\': out += "\\\\"; ++i; break;
        case '\n': out += "\\n"; ++i; break;
        case '\r': out += "\\r"; ++i; break;
        case '\t': out += "\\t"; ++i; break;
        case '\b': out += "\\b"; ++i; break;
        case '\f': out += "\\f"; ++i; break;
        default:
            if (ch < 0x20U) {
                detail::append_unicode_escape(out, ch);
                ++i;
            } else if (ch < 0x80U) {
                out += static_cast<char>(ch);
                ++i;
            } else {
                const std::size_t sequence_length =
                    detail::valid_utf8_sequence_length(value, i);
                if (sequence_length == 0U) {
                    detail::append_unicode_escape(out, ch);
                    ++i;
                } else {
                    out.append(value, i, sequence_length);
                    i += sequence_length;
                }
            }
            break;
        }
    }

    return out;
}

} // namespace netscope
