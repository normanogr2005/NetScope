#include "tcp.hpp"

#include <arpa/inet.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace netscope {
namespace {

std::string format_ipv4_hex(const std::string& value) {
    if (value.size() != 8U) {
        return "?";
    }
    std::uint32_t raw{};
    std::stringstream parser;
    parser << std::hex << value;
    parser >> raw;
    in_addr address{};
    address.s_addr = htonl(raw);
    char buffer[INET_ADDRSTRLEN]{};
    if (inet_ntop(AF_INET, &address, buffer, sizeof(buffer)) == nullptr) {
        return "?";
    }
    return buffer;
}

bool parse_endpoint(const std::string& endpoint, std::string& address, std::uint16_t& port) {
    const auto colon = endpoint.find(':');
    if (colon == std::string::npos) {
        return false;
    }
    const std::string address_hex = endpoint.substr(0, colon);
    const std::string port_hex = endpoint.substr(colon + 1);
    if (address_hex.size() != 8U) {
        return false;
    }

    unsigned int parsed_port{};
    std::stringstream port_stream;
    port_stream << std::hex << port_hex;
    port_stream >> parsed_port;
    if (port_stream.fail() || parsed_port > 65535U) {
        return false;
    }

    address = format_ipv4_hex(address_hex);
    port = static_cast<std::uint16_t>(parsed_port);
    return address != "?";
}

std::string state_name(const std::string& code) {
    static const std::vector<std::pair<std::string, std::string>> states{
        {"01", "ESTABLISHED"},
        {"02", "SYN_SENT"},
        {"03", "SYN_RECV"},
        {"04", "FIN_WAIT1"},
        {"05", "FIN_WAIT2"},
        {"06", "TIME_WAIT"},
        {"07", "CLOSE"},
        {"08", "CLOSE_WAIT"},
        {"09", "LAST_ACK"},
        {"0A", "LISTEN"},
        {"0B", "CLOSING"},
    };
    for (const auto& [key, name] : states) {
        if (key == code) {
            return name;
        }
    }
    return code;
}

std::vector<TcpConnection> read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return {};
    }

    std::vector<TcpConnection> result;
    std::string line;
    std::getline(file, line); // header
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string sl, local, remote, state;
        if (!(stream >> sl >> local >> remote >> state)) {
            continue;
        }

        TcpConnection connection;
        if (!parse_endpoint(local, connection.local_address, connection.local_port)) {
            continue;
        }
        if (!parse_endpoint(remote, connection.remote_address, connection.remote_port)) {
            continue;
        }
        connection.state = state_name(state);
        result.push_back(connection);
    }
    return result;
}

} // namespace

std::vector<TcpConnection> read_tcp_connections() {
    auto result = read_file("/proc/net/tcp");
    auto ipv6 = read_file("/proc/net/tcp6");
    result.insert(result.end(), ipv6.begin(), ipv6.end());
    return result;
}

} // namespace netscope
