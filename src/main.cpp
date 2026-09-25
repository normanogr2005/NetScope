#include "netstats.hpp"
#include "tcp.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Options {
    bool once{false};
    bool connections{false};
    double interval{1.0};
};

void print_help() {
    std::cout
        << "NORMANTOYS // NETSCOPE\n"
        << "Linux network visibility from the terminal.\n\n"
        << "Usage: netscope [options]\n\n"
        << "  -1, --once             Print one snapshot and exit\n"
        << "  -c, --connections      Show TCP connections\n"
        << "  -i, --interval SEC     Sampling interval (default: 1.0)\n"
        << "  -h, --help             Show this help\n";
}

bool parse_options(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-1" || arg == "--once") {
            options.once = true;
        } else if (arg == "-c" || arg == "--connections") {
            options.connections = true;
        } else if (arg == "-i" || arg == "--interval") {
            if (i + 1 >= argc) {
                std::cerr << "--interval requires a value\n";
                return false;
            }
            options.interval = std::strtod(argv[++i], nullptr);
            if (options.interval <= 0.0) {
                std::cerr << "interval must be > 0\n";
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

std::string human_rate(double bits_per_second) {
    constexpr double K = 1000.0;
    if (bits_per_second >= K * K * K) {
        return (std::to_string(bits_per_second / (K * K * K)).substr(0, 6) + " Gbps");
    }
    if (bits_per_second >= K * K) {
        return (std::to_string(bits_per_second / (K * K)).substr(0, 6) + " Mbps");
    }
    if (bits_per_second >= K) {
        return (std::to_string(bits_per_second / K).substr(0, 6) + " Kbps");
    }
    return (std::to_string(bits_per_second).substr(0, 6) + " bps");
}

void print_snapshot(const std::vector<netscope::RateStats>& rates) {
    std::cout << "\033[2J\033[H";
    std::cout << "┌──────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ NORMANTOYS // NETSCOPE                                      │\n";
    std::cout << "│ Linux network telemetry                                     │\n";
    std::cout << "└──────────────────────────────────────────────────────────────┘\n\n";
    std::cout << std::left << std::setw(14) << "INTERFACE"
              << std::right << std::setw(16) << "RX"
              << std::setw(16) << "TX" << "\n";
    std::cout << std::string(48, '-') << "\n";
    if (rates.empty()) {
        std::cout << "No interface samples available.\n";
        return;
    }
    for (const auto& rate : rates) {
        std::cout << std::left << std::setw(14) << rate.name
                  << std::right << std::setw(16) << human_rate(rate.rx_bps)
                  << std::setw(16) << human_rate(rate.tx_bps) << "\n";
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

        print_snapshot(rates);
        if (options.connections) {
            print_connections();
        }

        previous = current;
    } while (!options.once);

    return 0;
}
