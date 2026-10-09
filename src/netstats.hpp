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

struct InterfaceMetadata {
    std::string mac_address{"unknown"};
    std::string mtu{"unknown"};
    std::string operstate{"unknown"};
};

std::vector<InterfaceStats> read_interface_stats(const std::string& path = "/proc/net/dev");
std::vector<RateStats> calculate_rates(const std::vector<InterfaceStats>& previous,
                                       const std::vector<InterfaceStats>& current,
                                       double elapsed_seconds);

// Reads optional interface details from Linux sysfs. Missing fields are "unknown".
InterfaceMetadata read_interface_metadata(
    const std::string& interface_name,
    const std::string& sysfs_root = "/sys/class/net"
);

} // namespace netscope
