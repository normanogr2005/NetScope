#include "event_id.hpp"
#include "json_util.hpp"
#include "netstats.hpp"
#include "tcp.hpp"

#include <cerrno>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Options {
    bool once{false};
    bool connections{false};
    bool json{false};
    double interval{1.0};
    double alert_mbps{0.0};
};

void print_help() {
    std::cout
        << "NORMANTOYS // NETSCOPE\n"
        << "Linux network visibility from the terminal.\n\n"
        << "Usage: netscope [options]\n\n"
        << "  -1, --once             Print one snapshot and exit\n"
        << "  -c, --connections      Show TCP connections\n"
        << "  -j, --json             Emit Security-Lab compatible NDJSON\n"
        << "  -i, --interval SEC     Sampling interval (default: 1.0)\n"
        << "      --alert-mbps N     Warn when RX or TX reaches N Mbps\n"
        << "  -h, --help             Show this help\n";
}

bool parse_interval(const std::string& text, double& value) {
    errno = 0;
    char* end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);

    if (
        errno == ERANGE
        || end == text.c_str()
        || end == nullptr
        || *end != '\0'
        || !std::isfinite(parsed)
        || parsed <= 0.0
    ) {
        return false;
    }

    value = parsed;
    return true;
}

bool parse_options(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-1" || arg == "--once") {
            options.once = true;
        } else if (arg == "-c" || arg == "--connections") {
            options.connections = true;
        } else if (arg == "-j" || arg == "--json") {
            options.json = true;
        } else if (arg == "-i" || arg == "--interval") {
            if (i + 1 >= argc) {
                std::cerr << "--interval requires a value\n";
                return false;
            }

            const std::string value = argv[++i];
            if (!parse_interval(value, options.interval)) {
                std::cerr << "interval must be a finite number > 0\n";
                return false;
            }
        } else if (arg == "--alert-mbps") {
            if (i + 1 >= argc) {
                std::cerr << "--alert-mbps requires a value\n";
                return false;
            }

            const std::string value = argv[++i];
            if (
                !parse_interval(value, options.alert_mbps)
                || options.alert_mbps >
                    std::numeric_limits<double>::max() / 1000000.0
            ) {
                std::cerr << "alert threshold must be finite, > 0, and representable in bps\n";
                return false;
            }
        } else if (arg == "-h" || arg == "--help") {
            print_help();
            std::exit(0);
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return false;
        }
    }
    return true;
}

// Formats rates with stable precision without truncating significant digits.
std::string human_rate(double bits_per_second) {
    constexpr double K = 1000.0;
    std::ostringstream out;
    out << std::fixed << std::setprecision(2);

    if (bits_per_second >= K * K * K) {
        out << (bits_per_second / (K * K * K)) << " Gbps";
    } else if (bits_per_second >= K * K) {
        out << (bits_per_second / (K * K)) << " Mbps";
    } else if (bits_per_second >= K) {
        out << (bits_per_second / K) << " Kbps";
    } else {
        out << bits_per_second << " bps";
    }

    return out.str();
}

std::string timestamp_now() {
    const auto now = std::chrono::system_clock::now();
    const auto seconds =
        std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto micros =
        std::chrono::duration_cast<std::chrono::microseconds>(
            now - seconds)
            .count();

    const std::time_t time =
        std::chrono::system_clock::to_time_t(seconds);
    std::tm utc{};
    gmtime_r(&time, &utc);

    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(6) << micros
        << 'Z';
    return out.str();
}

void print_json_rate(const netscope::RateStats& rate, double alert_mbps) {
    const auto metadata = netscope::read_interface_metadata(rate.name);
    const bool alert = alert_mbps > 0.0
        && (rate.rx_bps >= alert_mbps * 1000000.0
            || rate.tx_bps >= alert_mbps * 1000000.0);
    std::cout << "{"
              << "\"event_id\":\"" << netscope::make_event_id("rate") << "\","
              << "\"timestamp\":\"" << timestamp_now() << "\","
              << "\"source\":\"netscope\","
              << "\"event_type\":\"network_interface_rate\","
              << "\"severity\":\"" << (alert ? "warning" : "info") << "\","
              << "\"message\":\"" << (alert ? "Interface traffic threshold exceeded" : "Interface traffic rate") << "\","
              << "\"metadata\":{\"interface\":\"" << netscope::json_escape(rate.name)
              << "\",\"rx_bps\":" << rate.rx_bps
              << ",\"tx_bps\":" << rate.tx_bps
              << ",\"mac_address\":\"" << netscope::json_escape(metadata.mac_address)
              << "\",\"mtu\":\"" << netscope::json_escape(metadata.mtu)
              << "\",\"operstate\":\"" << netscope::json_escape(metadata.operstate) << "\"}}\n";
}

void print_json_connection(const netscope::TcpConnection& c) {
    std::cout << "{"
              << "\"event_id\":\"" << netscope::make_event_id("tcp") << "\","
              << "\"timestamp\":\"" << timestamp_now() << "\","
              << "\"source\":\"netscope\","
              << "\"event_type\":\"tcp_connection\","
              << "\"severity\":\"info\","
              << "\"source_ip\":\"" << netscope::json_escape(c.local_address) << "\","
              << "\"destination_ip\":\"" << netscope::json_escape(c.remote_address) << "\","
              << "\"source_port\":" << c.local_port << ","
              << "\"destination_port\":" << c.remote_port << ","
              << "\"metadata\":{\"state\":\"" << netscope::json_escape(c.state) << "\"}}\n";
}

void print_snapshot(const std::vector<netscope::RateStats>& rates) {
    std::cout << "\033[2J\033[H";
    std::cout << "┌──────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ NORMANTOYS // NETSCOPE                                      │\n";
    std::cout << "│ Linux network telemetry                                     │\n";
    std::cout << "└──────────────────────────────────────────────────────────────┘\n\n";
    std::cout << std::left << std::setw(14) << "INTERFACE"
              << std::right << std::setw(16) << "RX"
              << std::setw(16) << "TX" << "  "
              << std::left << std::setw(8) << "STATE" << "  MAC / MTU" << "\n";
    std::cout << std::string(90, '-') << "\n";
    if (rates.empty()) {
        std::cout << "No interface samples available.\n";
        return;
    }
    for (const auto& rate : rates) {
        const auto metadata = netscope::read_interface_metadata(rate.name);
        std::cout << std::left << std::setw(14) << rate.name
                  << std::right << std::setw(16) << human_rate(rate.rx_bps)
                  << std::setw(16) << human_rate(rate.tx_bps)
                  << "  " << std::left << std::setw(8) << metadata.operstate
                  << "  " << metadata.mac_address
                  << "  MTU " << metadata.mtu << "\n";
    }
}

void print_connections() {
    const auto connections = netscope::read_tcp_connections();
    std::cout << "\nTCP CONNECTIONS\n";
    std::cout << std::left << std::setw(21) << "LOCAL"
              << std::setw(21) << "REMOTE"
              << "STATE\n";
    std::cout << std::string(58, '-') << "\n";
    for (const auto& c : connections) {
        const std::string local = c.local_address + ":" + std::to_string(c.local_port);
        const std::string remote = c.remote_address + ":" + std::to_string(c.remote_port);
        std::cout << std::left << std::setw(21) << local
                  << std::setw(21) << remote
                  << c.state << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_options(argc, argv, options)) {
        print_help();
        return 2;
    }

    auto previous = netscope::read_interface_stats();
    if (previous.empty()) {
        std::cerr << "Unable to read /proc/net/dev. Is this running on Linux?\n";
        return 1;
    }

    do {
        const auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::duration<double>(options.interval));
        const auto current = netscope::read_interface_stats();
        const auto end = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(end - start).count();
        const auto rates = netscope::calculate_rates(previous, current, elapsed);

        if (options.json) {
            for (const auto& rate : rates) {
                print_json_rate(rate, options.alert_mbps);
            }
            if (options.connections) {
                const auto connections = netscope::read_tcp_connections();
                for (const auto& connection : connections) {
                    print_json_connection(connection);
                }
            }
        } else {
            print_snapshot(rates);
            if (options.alert_mbps > 0.0) {
                const double threshold_bps = options.alert_mbps * 1000000.0;
                for (const auto& rate : rates) {
                    if (rate.rx_bps >= threshold_bps || rate.tx_bps >= threshold_bps) {
                        std::cerr << "WARNING: " << rate.name
                                  << " exceeded " << options.alert_mbps
                                  << " Mbps (RX " << human_rate(rate.rx_bps)
                                  << ", TX " << human_rate(rate.tx_bps) << ")\n";
                    }
                }
            }
            if (options.connections) {
                print_connections();
            }
        }

        previous = current;
    } while (!options.once);

    return 0;
}
