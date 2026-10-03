#include "netstats.hpp"
#include "tcp.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
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
        << "  -h, --help             Show this help\n";
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

std::string json_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char ch : value) {
        switch (ch) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += ch; break;
        }
    }
    return out;
}

std::string timestamp_now() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
    gmtime_r(&time, &utc);
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

void print_json_rate(const netscope::RateStats& rate) {
    std::cout << "{"
              << "\"event_id\":\"netscope-rate-" << std::hash<std::string>{}(rate.name)
              << "-" << std::time(nullptr) << "\","
              << "\"timestamp\":\"" << timestamp_now() << "\","
              << "\"source\":\"netscope\","
              << "\"event_type\":\"network_interface_rate\","
              << "\"severity\":\"info\","
              << "\"message\":\"Interface traffic rate\","
              << "\"metadata\":{\"interface\":\"" << json_escape(rate.name)
              << "\",\"rx_bps\":" << rate.rx_bps
              << ",\"tx_bps\":" << rate.tx_bps << "}}\n";
}

void print_json_connection(const netscope::TcpConnection& c) {
    std::cout << "{"
              << "\"event_id\":\"netscope-tcp-" << std::hash<std::string>{}(c.local_address + c.remote_address)
              << "-" << c.local_port << "-" << c.remote_port << "-" << std::time(nullptr) << "\","
              << "\"timestamp\":\"" << timestamp_now() << "\","
              << "\"source\":\"netscope\","
              << "\"event_type\":\"tcp_connection\","
              << "\"severity\":\"info\","
              << "\"source_ip\":\"" << json_escape(c.local_address) << "\","
              << "\"destination_ip\":\"" << json_escape(c.remote_address) << "\","
              << "\"source_port\":" << c.local_port << ","
              << "\"destination_port\":" << c.remote_port << ","
              << "\"metadata\":{\"state\":\"" << json_escape(c.state) << "\"}}\n";
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

        if (options.json) {
            for (const auto& rate : rates) {
                print_json_rate(rate);
            }
            if (options.connections) {
                const auto connections = netscope::read_tcp_connections();
                for (const auto& connection : connections) {
                    print_json_connection(connection);
                }
            }
        } else {
            print_snapshot(rates);
            if (options.connections) {
                print_connections();
            }
        }

        previous = current;
    } while (!options.once);

    return 0;
}
