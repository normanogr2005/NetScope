#include "../src/event_id.hpp"

#include <cassert>
#include <string>
#include <unordered_set>

int main() {
    std::unordered_set<std::string> ids;

    for (int i = 0; i < 1000; ++i) {
        ids.insert(netscope::make_event_id("tcp"));
    }

    assert(ids.size() == 1000);

    const auto rate_id = netscope::make_event_id("rate");
    const auto tcp_id = netscope::make_event_id("tcp");

    assert(!rate_id.empty());
    assert(!tcp_id.empty());
    assert(rate_id != tcp_id);

    return 0;
}
