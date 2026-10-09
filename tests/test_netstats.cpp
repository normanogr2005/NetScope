#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../src/netstats.hpp"
#include "../src/tcp.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <filesystem>
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
        out << "malformed: not enough fields\n";
        out << "this line has no colon\n";
    }

    const auto first = netscope::read_interface_stats(path);
    assert(first.size() == 2);
    assert(first[0].name == "lo");
    assert(first[1].rx_bytes == 5000);
    assert(first[1].tx_bytes == 7000);
    assert(netscope::read_interface_stats(path + ".missing").empty());

    const auto second = std::vector<netscope::InterfaceStats>{
        {"eth0", 9000, 11000},
        {"new0", 500, 700},
    };
    const auto rates = netscope::calculate_rates(first, second, 2.0);
    assert(rates.size() == 1);
    assert(rates[0].name == "eth0");
    assert(rates[0].rx_bps == 16000.0);
    assert(rates[0].tx_bps == 16000.0);
    assert(netscope::calculate_rates(first, second, 0.0).empty());
    assert(netscope::calculate_rates(first, second, -1.0).empty());

    const auto reset = std::vector<netscope::InterfaceStats>{{"eth0", 100, 200}};
    const auto reset_rates = netscope::calculate_rates(first, reset, 1.0);
    assert(reset_rates.size() == 1);
    assert(reset_rates[0].rx_bps == 0.0);
    assert(reset_rates[0].tx_bps == 0.0);

    const std::filesystem::path sysfs_root = "netscope_test_sysfs";
    const auto interface_dir = sysfs_root / "eth-test";
    std::filesystem::create_directories(interface_dir);
    {
        std::ofstream(interface_dir / "address") << "02:00:00:00:00:01\n";
        std::ofstream(interface_dir / "mtu") << "1500\n";
        std::ofstream(interface_dir / "operstate") << "up\n";
    }
    const auto metadata = netscope::read_interface_metadata("eth-test", sysfs_root.string());
    assert(metadata.mac_address == "02:00:00:00:00:01");
    assert(metadata.mtu == "1500");
    assert(metadata.operstate == "up");
    const auto missing_metadata = netscope::read_interface_metadata("missing0", sysfs_root.string());
    assert(missing_metadata.mac_address == "unknown");
    assert(netscope::read_interface_metadata("../etc", sysfs_root.string()).mtu == "unknown");
    std::filesystem::remove_all(sysfs_root);

    const std::string tcp_path = "netscope_test_tcp.txt";
    const std::string tcp6_path = "netscope_test_tcp6.txt";

    {
        std::ofstream out(tcp_path);
        out << "  sl  local_address rem_address   st\n";
        out << "   0: 0100007F:0016 0200007F:C350 01\n";
        out << "   1: 6401A8C0:0050 0100007F:0016 0A\n";
        out << "   2: 0100007F:0016X 0200007F:C350 01\n";
        out << "   3: 0100007F:GGGG 0200007F:C350 01\n";
        out << "   4: 0100007G:0016 0200007F:C350 01\n";
    }

    {
        std::ofstream out(tcp6_path);
        out << "  sl  local_address rem_address   st\n";
        out << "   0: 00000000000000000000000001000000:0016 00000000000000000000000002000000:C350 01\n";
    }

    const auto connections =
        netscope::read_tcp_connections(tcp_path, tcp6_path);

    assert(connections.size() == 3);

    assert(connections[0].local_address == "127.0.0.1");
    assert(connections[0].remote_address == "127.0.0.2");
    assert(connections[0].local_port == 22);
    assert(connections[0].remote_port == 50000);
    assert(connections[0].state == "ESTABLISHED");

    assert(connections[1].local_address == "192.168.1.100");
    assert(connections[1].remote_address == "127.0.0.1");
    assert(connections[1].local_port == 80);
    assert(connections[1].remote_port == 22);
    assert(connections[1].state == "LISTEN");

    assert(connections[2].local_address == "::1");
    assert(connections[2].remote_address == "::2");
    assert(connections[2].local_port == 22);
    assert(connections[2].remote_port == 50000);
    assert(connections[2].state == "ESTABLISHED");

    std::remove(path.c_str());
    std::remove(tcp_path.c_str());
    std::remove(tcp6_path.c_str());
    return 0;
}
