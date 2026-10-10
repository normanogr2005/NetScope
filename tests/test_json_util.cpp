#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../src/json_util.hpp"

#include <cassert>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

std::string unicode_escape(unsigned int code) {
    std::ostringstream expected;
    expected << "\\u" << std::hex << std::nouppercase
             << std::setfill('0') << std::setw(4) << code;
    return expected.str();
}

} // namespace

int main() {
    using netscope::json_escape;

    assert(json_escape("") == "");
    assert(json_escape("eth0") == "eth0");
    assert(json_escape("plain text") == "plain text");

    // Valid UTF-8 sequences, including 2-, 3-, and 4-byte code points, pass through.
    assert(json_escape(std::string("caf\xC3\xA9")) == std::string("caf\xC3\xA9"));
    assert(json_escape(std::string("\xE2\x82\xAC")) == std::string("\xE2\x82\xAC"));
    assert(json_escape(std::string("\xF0\x9F\x94\x90")) == std::string("\xF0\x9F\x94\x90"));

    // Named JSON escapes.
    assert(json_escape("\"quoted\"") == "\\\"quoted\\\"");
    assert(json_escape("back\\slash") == "back\\\\slash");
    assert(json_escape("line\nbreak") == "line\\nbreak");
    assert(json_escape("carriage\rreturn") == "carriage\\rreturn");
    assert(json_escape("a\tb") == "a\\tb");
    assert(json_escape("a\bb") == "a\\bb");
    assert(json_escape("a\fb") == "a\\fb");

    // Every C0 control character is escaped, using short forms where available.
    for (unsigned int code = 0; code < 0x20U; ++code) {
        std::string expected;
        switch (code) {
        case 0x08U: expected = "\\b"; break;
        case 0x09U: expected = "\\t"; break;
        case 0x0AU: expected = "\\n"; break;
        case 0x0CU: expected = "\\f"; break;
        case 0x0DU: expected = "\\r"; break;
        default: expected = unicode_escape(code); break;
        }
        const std::string input(1, static_cast<char>(code));
        assert(json_escape(input) == expected);
    }

    assert(json_escape(std::string("mtu:\x01 1500")) == "mtu:\\u0001 1500");
    assert(json_escape(" ") == " ");
    assert(json_escape("\x7F") == "\x7F");

    // Malformed UTF-8 is escaped byte-by-byte so output remains valid UTF-8.
    assert(json_escape(std::string(1, static_cast<char>(0x80))) == "\\u0080");
    assert(json_escape(std::string(1, static_cast<char>(0xC0))) == "\\u00c0");
    assert(json_escape(std::string("\xC0\xAF")) == "\\u00c0\\u00af");
    assert(json_escape(std::string("\xE2\x82")) == "\\u00e2\\u0082");
    assert(json_escape(std::string("\xF4\x90\x80\x80")) ==
           "\\u00f4\\u0090\\u0080\\u0080");
    assert(json_escape(std::string("x") + static_cast<char>(0xFF) + "y") ==
           "x\\u00ffy");

    return 0;
}
