#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace netscope {

inline std::string make_event_id(const std::string& kind) {
    static const std::string session_id = [] {
        std::random_device device;
        const std::uint64_t entropy =
            (static_cast<std::uint64_t>(device()) << 32U) ^
            static_cast<std::uint64_t>(device());
        const auto clock_value =
            static_cast<std::uint64_t>(
                std::chrono::high_resolution_clock::now()
                    .time_since_epoch()
                    .count());

        std::ostringstream out;
        out << std::hex << std::setfill('0')
            << std::setw(16) << clock_value
            << std::setw(16) << entropy;
        return out.str();
    }();

    static std::atomic<std::uint64_t> sequence{0};
    const auto sequence_number =
        sequence.fetch_add(1, std::memory_order_relaxed);

    return "netscope-" + kind + "-" + session_id + "-" +
           std::to_string(sequence_number);
}

} // namespace netscope
