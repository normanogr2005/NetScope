#include "../src/netstats.hpp"
#include "../src/tcp.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

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

    const std::string tcp_path = "netscope_test_tcp.txt";
    const std::string tcp6_path = "netscope_test_tcp6.txt";

    {
        std::ofstream out(tcp_path);
        out << "  sl  local_address rem_address   st\n";
        out << "   0: 0100007F:0016 0200007F:C350 01\n";
    }

    {
        std::ofstream out(tcp6_path);
        out << "  sl  local_address rem_address   st\n";
        out << "   0: 00000000000000000000000001000000:0016 00000000000000000000000002000000:C350 01\n";
    }

    const auto connections =
        netscope::read_tcp_connections(tcp_path, tcp6_path);

    assert(connections.size() == 2);

    assert(connections[0].local_address == "127.0.0.1");
    assert(connections[0].remote_address == "127.0.0.2");
    assert(connections[0].local_port == 22);
    assert(connections[0].remote_port == 50000);
    assert(connections[0].state == "ESTABLISHED");

    assert(connections[1].local_address == "::1");
    assert(connections[1].remote_address == "::2");
    assert(connections[1].local_port == 22);
    assert(connections[1].remote_port == 50000);
    assert(connections[1].state == "ESTABLISHED");

    std::remove(path.c_str());
    std::remove(tcp_path.c_str());
    std::remove(tcp6_path.c_str());
    return 0;
}
