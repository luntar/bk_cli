#include "Command_Helpers.h"

#include <charconv>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace bk;

namespace {

template <typename T>
T parse_signed_bounded(const std::string& str, T default_value)
{
    if (str.empty()) {
        return default_value;
    }

    long long value{};
    const auto* begin = str.data();
    const auto* end = begin + str.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc()
        || ptr != end
        || value < static_cast<long long>(std::numeric_limits<T>::min())
        || value > static_cast<long long>(std::numeric_limits<T>::max())) {
        return default_value;
    }

    return static_cast<T>(value);
}

template <typename T>
T parse_unsigned_bounded(const std::string& str, T default_value, int base = 10)
{
    if (str.empty()) {
        return default_value;
    }

    unsigned long long value{};
    const auto* begin = str.data();
    const auto* end = begin + str.size();
    auto [ptr, ec] = std::from_chars(begin, end, value, base);
    if (ec != std::errc()
        || ptr != end
        || value > static_cast<unsigned long long>(std::numeric_limits<T>::max())) {
        return default_value;
    }

    return static_cast<T>(value);
}

} // namespace

std::size_t Command_Helpers::count_tokens(std::string_view sv)
{
    std::size_t n = 0;
    bool in_token = false;

    for (char c : sv) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isspace(uc)) {
            in_token = false;
        }
        else {
            if (!in_token) { ++n; }
            in_token = true;
        }
    }
    return n;
}

uint64_t Command_Helpers::to_uint64(const std::string& str, uint64_t default_value)
{
    return parse_unsigned_bounded<uint64_t>(str, default_value);
}

int64_t Command_Helpers::to_int64(const std::string& str, int64_t default_value)
{
    return parse_signed_bounded<int64_t>(str, default_value);
}

uint32_t Command_Helpers::to_uint32(const std::string& str, uint32_t default_value)
{
    return parse_unsigned_bounded<uint32_t>(str, default_value);
}

int32_t Command_Helpers::to_int32(const std::string& str, int32_t default_value)
{
    return parse_signed_bounded<int32_t>(str, default_value);
}

uint16_t Command_Helpers::to_uint16(const std::string& str, uint16_t default_value)
{
    return parse_unsigned_bounded<uint16_t>(str, default_value);
}

int16_t Command_Helpers::to_int16(const std::string& str, int16_t default_value)
{
    return parse_signed_bounded<int16_t>(str, default_value);
}

uint8_t Command_Helpers::to_uint8(const std::string& str, uint8_t default_value)
{
    return parse_unsigned_bounded<uint8_t>(str, default_value);
}

int8_t Command_Helpers::to_int8(const std::string& str, int8_t default_value)
{
    return parse_signed_bounded<int8_t>(str, default_value);
}

char Command_Helpers::to_hex_char(const std::string& str, char default_value)
{
    return parse_unsigned_bounded<char>(str, default_value, 16);
}

char Command_Helpers::to_char(const std::string& str, char default_value)
{
    return parse_unsigned_bounded<char>(str, default_value);
}

float Command_Helpers::to_float(const std::string& str, float default_value)
{
    if (str.empty()) {
        return default_value;
    }

    float val{};
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);
    if (ec == std::errc() && ptr == str.data() + str.size()) {
        return val;
    }

    return default_value;
}

double Command_Helpers::to_double(const std::string& str, double default_value)
{
    if (str.empty()) {
        return default_value;
    }

    double val{};
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);
    if (ec == std::errc() && ptr == str.data() + str.size()) {
        return val;
    }

    return default_value;
}

std::string Command_Helpers::to_string(const std::string& str, const std::string& default_value)
{
    return str.empty() ? default_value : str;
}

std::vector<std::string_view> Command_Helpers::tokenize(std::string_view sv)
{
    std::vector<std::string_view> tokens;
    std::size_t i = 0;
    while (i < sv.size()) {
        while (i < sv.size() && std::isspace(static_cast<unsigned char>(sv[i]))) {
            ++i;
        }

        const std::size_t start = i;
        while (i < sv.size() && !std::isspace(static_cast<unsigned char>(sv[i]))) {
            ++i;
        }

        if (start < i) {
            tokens.emplace_back(sv.substr(start, i - start));
        }
    }
    return tokens;
}

std::string Command_Helpers::remove_first(const std::string& args)
{
    auto pos = args.find_first_of(" \t");
    if (pos == std::string::npos) {
        return {};
    }

    auto next = args.find_first_not_of(" \t", pos);
    if (next == std::string::npos) {
        return {};
    }
    return args.substr(next);
}

bool Command_Helpers::is_optional(argument_type t)
{
    switch (t) {
    case argument_type::u64_opt:
    case argument_type::i64_opt:
    case argument_type::u32_opt:
    case argument_type::i32_opt:
    case argument_type::u16_opt:
    case argument_type::i16_opt:
    case argument_type::f32_opt:
    case argument_type::f64_opt:
    case argument_type::u8_opt:
    case argument_type::i8_opt:
    case argument_type::str_opt:
        return true;
    default:
        return false;
    }
}

std::string Command_Helpers::argument_type_to_string(argument_type t)
{
    if (t == argument_type::unknown) {
        return "unknown";
    }
    else if (t == argument_type::u64 || t == argument_type::u64_opt) {
        return "u64";
    }
    else if (t == argument_type::i64 || t == argument_type::i64_opt) {
        return "i64";
    }
    else if (t == argument_type::u32 || t == argument_type::u32_opt) {
        return "u32";
    }
    else if (t == argument_type::i32 || t == argument_type::i32_opt) {
        return "i32";
    }
    else if (t == argument_type::u16 || t == argument_type::u16_opt) {
        return "u16";
    }
    else if (t == argument_type::i16 || t == argument_type::i16_opt) {
        return "i16";
    }
    else if (t == argument_type::f32 || t == argument_type::f32_opt) {
        return "f32";
    }
    else if (t == argument_type::f64 || t == argument_type::f64_opt) {
        return "f64";
    }
    else if (t == argument_type::u8 || t == argument_type::u8_opt) {
        return "u8";
    }
    else if (t == argument_type::i8 || t == argument_type::i8_opt) {
        return "i8";
    }
    else if (t == argument_type::str || t == argument_type::str_opt) {
        return "str";
    }
    return "argument_type_not_defined";
}

std::string Command_Helpers::escape_json_string(const std::string& input)
{
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
        case '\"': ss << "\\\""; break;
        case '\\': ss << "\\\\"; break;
        case '\b': ss << "\\b";  break;
        case '\f': ss << "\\f";  break;
        case '\n': ss << "\\n";  break;
        case '\r': ss << "\\r";  break;
        case '\t': ss << "\\t";  break;
        default:
            if (static_cast<unsigned char>(c) <= 0x1F) {
                ss << "\\u"
                    << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(c);
            }
            else {
                ss << c;
            }
        }
    }
    return ss.str();
}
