#include "netstats.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cctype>

namespace netscope {

std::vector<InterfaceStats> read_interface_stats(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return {};
    }

    std::vector<InterfaceStats> result;
    std::string line;
    while (std::getline(file, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string name = line.substr(0, colon);
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.front())) != 0) {
            name.erase(name.begin());
        }
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back())) != 0) {
            name.pop_back();
        }
        std::string payload = line.substr(colon + 1);
        std::istringstream stream(payload);
        InterfaceStats stats;
        stats.name = name;

        // /proc/net/dev: RX bytes is field 1; TX bytes is field 9.
        std::uint64_t rx_packets{}, rx_errs{}, rx_drop{}, rx_fifo{}, rx_frame{}, rx_compressed{}, rx_multicast{};
        std::uint64_t tx_packets{}, tx_errs{}, tx_drop{}, tx_fifo{}, tx_colls{}, tx_carrier{}, tx_compressed{};
        if (!(stream >> stats.rx_bytes >> rx_packets >> rx_errs >> rx_drop >> rx_fifo >> rx_frame >> rx_compressed >> rx_multicast
                     >> stats.tx_bytes >> tx_packets >> tx_errs >> tx_drop >> tx_fifo >> tx_colls >> tx_carrier >> tx_compressed)) {
            continue;
        }

        // Ignore malformed interface names and keep the parser predictable.
        if (stats.name.empty()) {
            continue;
        }
        result.push_back(stats);
    }

    return result;
}

std::vector<RateStats> calculate_rates(const std::vector<InterfaceStats>& previous,
                                       const std::vector<InterfaceStats>& current,
                                       double elapsed_seconds) {
    if (elapsed_seconds <= 0.0) {
        return {};
    }

    std::unordered_map<std::string, InterfaceStats> previous_by_name;
    for (const auto& item : previous) {
        previous_by_name[item.name] = item;
    }

    std::vector<RateStats> result;
    result.reserve(current.size());

    for (const auto& item : current) {
        const auto found = previous_by_name.find(item.name);
        if (found == previous_by_name.end()) {
            continue;
        }

        const auto rx_delta = item.rx_bytes >= found->second.rx_bytes
            ? item.rx_bytes - found->second.rx_bytes
            : 0U;
        const auto tx_delta = item.tx_bytes >= found->second.tx_bytes
            ? item.tx_bytes - found->second.tx_bytes
            : 0U;

        RateStats rate;
        rate.name = item.name;
        rate.rx_bps = (static_cast<double>(rx_delta) * 8.0) / elapsed_seconds;
        rate.tx_bps = (static_cast<double>(tx_delta) * 8.0) / elapsed_seconds;
        result.push_back(rate);
    }

    std::sort(result.begin(), result.end(), [](const RateStats& a, const RateStats& b) {
        return (a.rx_bps + a.tx_bps) > (b.rx_bps + b.tx_bps);
    });
    return result;
}

} // namespace netscope
