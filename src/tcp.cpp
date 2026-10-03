#include "tcp.hpp"

#include <arpa/inet.h>
#include <array>
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

    if (parser.fail()) {
        return "?";
    }

    in_addr address{};
    address.s_addr = htonl(raw);

    char buffer[INET_ADDRSTRLEN]{};
    if (inet_ntop(AF_INET, &address, buffer, sizeof(buffer)) == nullptr) {
        return "?";
    }

    return buffer;
}

std::string format_ipv6_hex(const std::string& value) {
    if (value.size() != 32U) {
        return "?";
    }

    std::array<unsigned char, 16> bytes{};

    // /proc/net/tcp6 stores each 32-bit word in host byte order.
    for (std::size_t word = 0; word < 4U; ++word) {
        for (std::size_t byte = 0; byte < 4U; ++byte) {
            const std::size_t source = word * 8U + (3U - byte) * 2U;
            const std::string hex_byte = value.substr(source, 2U);

            unsigned int parsed{};
            std::stringstream parser;
            parser << std::hex << hex_byte;
            parser >> parsed;

            if (parser.fail() || parsed > 255U) {
                return "?";
            }

            bytes[word * 4U + byte] =
                static_cast<unsigned char>(parsed);
        }
    }

    char buffer[INET6_ADDRSTRLEN]{};
    if (inet_ntop(AF_INET6, bytes.data(), buffer, sizeof(buffer)) == nullptr) {
        return "?";
    }

    return buffer;
}

bool parse_endpoint(
    const std::string& endpoint,
    int family,
    std::string& address,
    std::uint16_t& port
) {
    const auto colon = endpoint.find(':');
    if (colon == std::string::npos) {
        return false;
    }

    const std::string address_hex = endpoint.substr(0, colon);
    const std::string port_hex = endpoint.substr(colon + 1);

    std::string parsed_address;
    if (family == AF_INET) {
        parsed_address = format_ipv4_hex(address_hex);
    } else if (family == AF_INET6) {
        parsed_address = format_ipv6_hex(address_hex);
    } else {
        return false;
    }

    if (parsed_address == "?") {
        return false;
    }

    unsigned int parsed_port{};
    std::stringstream port_stream;
    port_stream << std::hex << port_hex;
    port_stream >> parsed_port;

    if (
        port_stream.fail()
        || parsed_port > 65535U
        || port_hex.empty()
    ) {
        return false;
    }

    address = parsed_address;
    port = static_cast<std::uint16_t>(parsed_port);
    return true;
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

std::vector<TcpConnection> read_file(
    const std::string& path,
    int family
) {
    std::ifstream file(path);
    if (!file) {
        return {};
    }

    std::vector<TcpConnection> result;
    std::string line;
    std::getline(file, line); // header

    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string sl;
        std::string local;
        std::string remote;
        std::string state;

        if (!(stream >> sl >> local >> remote >> state)) {
            continue;
        }

        TcpConnection connection;

        if (!parse_endpoint(
                local,
                family,
                connection.local_address,
                connection.local_port)) {
            continue;
        }

        if (!parse_endpoint(
                remote,
                family,
                connection.remote_address,
                connection.remote_port)) {
            continue;
        }

        connection.state = state_name(state);
        result.push_back(connection);
    }

    return result;
}

} // namespace

std::vector<TcpConnection> read_tcp_connections(
    const std::string& tcp_path,
    const std::string& tcp6_path
) {
    auto result = read_file(tcp_path, AF_INET);
    auto ipv6 = read_file(tcp6_path, AF_INET6);
    result.insert(result.end(), ipv6.begin(), ipv6.end());
    return result;
}

std::vector<TcpConnection> read_tcp_connections() {
    return read_tcp_connections("/proc/net/tcp", "/proc/net/tcp6");
}

} // namespace netscope
