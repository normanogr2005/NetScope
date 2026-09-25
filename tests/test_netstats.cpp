#include "../src/netstats.hpp"

#include <cassert>
#include <fstream>
#include <string>

int main() {
    const std::string path = "netscope_test_netdev.txt";
    {
        std::ofstream out(path);
        out << "Inter-|   Receive                                                |  Transmit\n";
        out << " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n";
        out << "  lo: 1000 10 0 0 0 0 0 0 2000 20 0 0 0 0 0 0\n";
        out << "eth0: 5000 50 0 0 0 0 0 0 7000 70 0 0 0 0 0 0\n";
    }

    const auto first = netscope::read_interface_stats(path);
    assert(first.size() == 2);
    assert(first[0].name == "lo");
    assert(first[1].rx_bytes == 5000);
    assert(first[1].tx_bytes == 7000);

    const auto second = std::vector<netscope::InterfaceStats>{{"eth0", 9000, 11000}};
    const auto rates = netscope::calculate_rates(first, second, 2.0);
    assert(rates.size() == 1);
    assert(rates[0].name == "eth0");
    assert(rates[0].rx_bps == 16000.0);
    assert(rates[0].tx_bps == 16000.0);

    std::remove(path.c_str());
    return 0;
}
