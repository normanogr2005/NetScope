#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace netscope {

struct InterfaceStats {
    std::string name;
    std::uint64_t rx_bytes{};
    std::uint64_t tx_bytes{};
};

struct RateStats {
    std::string name;
    double rx_bps{};
    double tx_bps{};
};

std::vector<InterfaceStats> read_interface_stats(const std::string& path = "/proc/net/dev");
std::vector<RateStats> calculate_rates(const std::vector<InterfaceStats>& previous,
                                       const std::vector<InterfaceStats>& current,
                                       double elapsed_seconds);

} // namespace netscope
