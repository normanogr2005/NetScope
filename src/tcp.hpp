#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace netscope {

struct TcpConnection {
    std::string local_address;
    std::uint16_t local_port{};
    std::string remote_address;
    std::uint16_t remote_port{};
    std::string state;
};

std::vector<TcpConnection> read_tcp_connections();

} // namespace netscope
