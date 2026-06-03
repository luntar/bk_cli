#include "Command.h"

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

using namespace bk;

namespace {

std::optional<std::string> json_string_field(const std::string& json, const std::string& field)
{
    const std::string key = "\"" + field + "\"";
    auto pos = json.find(key);
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    pos = json.find(':', pos + key.size());
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    std::string value;
    bool escaped = false;
    for (++pos; pos < json.size(); ++pos) {
        const char c = json[pos];
        if (escaped) {
            switch (c) {
            case '"': value.push_back('"'); break;
            case '\\': value.push_back('\\'); break;
            case '/': value.push_back('/'); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            default: value.push_back(c); break;
            }
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            return value;
        }

        value.push_back(c);
    }

    return std::nullopt;
}

template <typename T>
bool parse_integer(std::string_view s)
{
    if (s.empty()) {
        return false;
    }

    T value{};
    const auto* begin = s.data();
    const auto* end = begin + s.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc() && ptr == end;
}

bool parse_float(std::string_view s)
{
    if (s.empty()) {
        return false;
    }

    double value{};
    const auto* begin = s.data();
    const auto* end = begin + s.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc() && ptr == end;
}

template <typename T>
bool parse_integer_range(std::string_view s)
{
    long long value{};
    const auto* begin = s.data();
    const auto* end = begin + s.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc()
        && ptr == end
        && value >= static_cast<long long>(std::numeric_limits<T>::min())
        && value <= static_cast<long long>(std::numeric_limits<T>::max());
}

template <typename T>
bool parse_unsigned_range(std::string_view s)
{
    unsigned long long value{};
    const auto* begin = s.data();
    const auto* end = begin + s.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc()
        && ptr == end
        && value <= static_cast<unsigned long long>(std::numeric_limits<T>::max());
}

argument_type required_type(argument_type t)
{
    switch (t) {
    case argument_type::u64_opt: return argument_type::u64;
    case argument_type::i64_opt: return argument_type::i64;
    case argument_type::u32_opt: return argument_type::u32;
    case argument_type::i32_opt: return argument_type::i32;
    case argument_type::u16_opt: return argument_type::u16;
    case argument_type::i16_opt: return argument_type::i16;
    case argument_type::f32_opt: return argument_type::f32;
    case argument_type::f64_opt: return argument_type::f64;
    case argument_type::u8_opt: return argument_type::u8;
    case argument_type::i8_opt: return argument_type::i8;
    case argument_type::str_opt: return argument_type::str;
    default: return t;
    }
}

bool is_valid_arg_value(argument_type t, std::string_view value)
{
    switch (required_type(t)) {
    case argument_type::unknown: return false;
    case argument_type::u64: return parse_unsigned_range<uint64_t>(value);
    case argument_type::i64: return parse_integer<int64_t>(value);
    case argument_type::u32: return parse_unsigned_range<uint32_t>(value);
    case argument_type::i32: return parse_integer_range<int32_t>(value);
    case argument_type::u16: return parse_unsigned_range<uint16_t>(value);
    case argument_type::i16: return parse_integer_range<int16_t>(value);
    case argument_type::f32:
    case argument_type::f64: return parse_float(value);
    case argument_type::u8: return parse_unsigned_range<uint8_t>(value);
    case argument_type::i8: return parse_integer_range<int8_t>(value);
    case argument_type::str: return true;
    default: return false;
    }
}

std::string bool_to_string(bool value)
{
    return value ? "true" : "false";
}

std::optional<bool> parse_bool(std::string_view value)
{
    if (value == "1" || value == "true" || value == "on" || value == "yes") {
        return true;
    }
    if (value == "0" || value == "false" || value == "off" || value == "no") {
        return false;
    }
    return std::nullopt;
}

} // namespace

std::string cmd_return_t::make_result_str(const std::string& result, const std::string& msg)
{
    return R"({"result":")" + result + R"(","msg":")"
        + Command_Helpers::escape_json_string(msg) + R"("})";
}

Command::Command()
{
    command_t cmd;
    cmd.name = "help";
    cmd.doc = "Print the help message";
    cmd.h = [this](args_t) {
        cmd_return_t rtval;
        std::ostringstream oss;

        for (const auto& e : _commands) {
            const command_t& cmd = e.second;
            if (!cmd.is_valid()) {
                continue;
            }

            std::string line = build_sub_cmd_name(cmd.name);
            for (std::size_t i = 0; i < cmd.args.size(); ++i) {
                const auto arg = cmd.args[i];
                const std::string arg_open = Command_Helpers::is_optional(arg) ? " [<" : " <";
                const std::string arg_close = Command_Helpers::is_optional(arg) ? ">]" : ">";
                const std::string arg_doc = cmd.get_arg_doc(i);
                line += arg_open + arg_doc + arg_close;
            }

            const std::size_t pad_width = (line.length() < k_help_name_width)
                ? (k_help_name_width - line.length())
                : 1;
            oss << line << std::string(pad_width, ' ') << "- " << cmd.doc << "\n";
        }

        rtval.set_success(oss.str());
        return rtval;
    };

    add(cmd);
}

void Command::add(command_t cmd) noexcept
{
    _commands[cmd.name] = cmd;
}

void Command::add(
    std::string name,
    std::string doc,
    handler_func h,
    arg_def_t args,
    std::vector<std::string> args_doc) noexcept
{
    command_t cmd;
    cmd.name = std::move(name);
    cmd.doc = std::move(doc);
    cmd.h = std::move(h);
    cmd.args = std::move(args);
    cmd.args_doc = std::move(args_doc);
    add(std::move(cmd));
}

void Command::bind(std::string name, std::string doc, bool& value) noexcept
{
    add(
        std::move(name),
        std::move(doc),
        [&value](args_t args) {
            cmd_return_t result;
            if (!args.empty()) {
                const auto parsed = parse_bool(args.front().value);
                if (!parsed) {
                    result.set_error("Expected bool: true false on off 1 0");
                    return result;
                }
                value = *parsed;
            }

            result.set_success(bool_to_string(value));
            return result;
        },
        { argument_type::str_opt },
        { "value" });
}

void Command::bind(std::string name, std::string doc, uint32_t& value) noexcept
{
    add(
        std::move(name),
        std::move(doc),
        [&value](args_t args) {
            if (!args.empty()) {
                value = Command_Helpers::to_uint32(args.front().value, value);
            }

            cmd_return_t result;
            result.set_success(std::to_string(value));
            return result;
        },
        { argument_type::u32_opt },
        { "value" });
}

command_t Command::get(std::string n) const noexcept
{
    if (auto it = _commands.find(n); it != _commands.end()) {
        return it->second;
    }
    return {};
}

command_t* Command::get_ptr(const std::string& n) noexcept
{
    auto it = _commands.find(n);
    if (it != _commands.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string Command::get_doc(std::string n) const noexcept
{
    auto cmd = get(n);
    return cmd.is_valid() ? cmd.doc : std::string{};
}

std::string Command::handler(const std::string& cmd_str)
{
    std::string rtval(R"({"result":"ERROR","msg":"Unknown"})");

    const auto cmd = json_string_field(cmd_str, "cmd");
    const auto args = json_string_field(cmd_str, "value");
    if (!cmd || !args) {
        rtval = std::string(R"({"result":"ERROR","msg":"Missing cmd"})");
    }
    else {
        if (_locked) {
            rtval = std::string(R"({"result":"ERROR","msg":"Locked"})");
        }
        else {
            rtval = execute(*cmd, *args);
        }
    }

    return rtval;
}

std::string Command::execute(const std::string& cmd_str, const std::string& args_in)
{
    command_t cmd = get(cmd_str);
    cmd_return_t rtval;
    if (!cmd.is_valid()) {
        rtval = { false, "Command not found" };
    }
    else {
        std::string args = args_in;
        std::size_t arg_count = Command_Helpers::count_tokens(args);

        if (cmd.is_parent()) {
            auto cs = first_arg(args);
            auto sub_cmd = get(cs);
            if (sub_cmd.is_valid() && sub_cmd.parent == cmd.name) {
                const auto sub_arg_cnt = sub_cmd.required_arg_count();
                if (arg_count > sub_arg_cnt) {
                    cmd = sub_cmd;
                    take_first_arg(args);
                    arg_count--;
                }
            }
        }

        const std::size_t required_count = cmd.required_arg_count();
        const std::size_t max_count = cmd.args.size();
        if (arg_count < required_count) {
            rtval = { false, "Too few arguments" };
        }
        else if (arg_count > max_count) {
            rtval = { false, "Too many arguments" };
        }
        else if (!cmd.h) {
            rtval = { false, "Command has no handler" };
        }
        else {
            auto a = make_args_from_string(args, cmd.args);
            for (std::size_t i = 0; i < a.size(); ++i) {
                if (!is_valid_arg_value(a[i].type, a[i].value)) {
                    const auto expected = Command_Helpers::argument_type_to_string(a[i].type);
                    const auto name = cmd.get_arg_doc(i);
                    rtval = { false, "Invalid argument '" + name + "': expected " + expected };
                    break;
                }
            }

            if (rtval.is_success()) {
                rtval = cmd.h(a);
            }
        }
    }

    return rtval.to_string();
}

std::string Command::execute_line(const std::string& line)
{
    const auto start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        cmd_return_t rtval{ false, "Missing command" };
        return rtval.to_string();
    }

    const auto cmd_end = line.find_first_of(" \t\r\n", start);
    if (cmd_end == std::string::npos) {
        return execute(line.substr(start), "");
    }

    const auto args_start = line.find_first_not_of(" \t\r\n", cmd_end);
    const std::string args = (args_start == std::string::npos) ? std::string{} : line.substr(args_start);
    return execute(line.substr(start, cmd_end - start), args);
}

bk::args_t Command::make_args_from_string(const std::string& args, const arg_def_t& def)
{
    args_t rtval;
    std::size_t cnt = 0;
    auto toks = Command_Helpers::tokenize(args);

    for (const auto& e : toks) {
        arg_t aa;
        aa.type = (cnt < def.size()) ? def[cnt] : argument_type::unknown;
        aa.value = std::string(e);
        rtval.push_back(aa);
        cnt++;
    }
    return rtval;
}

void Command::take_first_arg(std::string& args)
{
    auto toks = Command_Helpers::tokenize(args);
    if (!toks.empty()) {
        args = Command_Helpers::remove_first(args);
    }
}

std::string Command::first_arg(const std::string& args)
{
    auto toks = Command_Helpers::tokenize(args);
    return toks.empty() ? std::string{} : std::string(toks.front());
}

void Command::add_child(command_t& parent, command_t subcmd) noexcept
{
    command_t* parent_ptr = get_ptr(parent.name);
    if (parent_ptr) {
        subcmd.set_parent(*parent_ptr);
    }

    add(subcmd);
}

std::string Command::build_sub_cmd_name(std::string n, const std::string& accumulated) const
{
    std::string rtval;
    command_t c = get(n);
    if (!c.parent.empty()) {
        if (accumulated.empty()) {
            rtval = build_sub_cmd_name(c.parent, n);
        }
        else {
            rtval = build_sub_cmd_name(c.parent, std::string(n + " " + accumulated));
        }
    }
    else {
        rtval = accumulated.empty() ? n : n + " " + accumulated;
    }

    return rtval;
}

void Command::set_locked(bool locked)
{
    const bool last_state = _locked;

    _locked = locked;

    if (last_state != _locked && _lock_changed_cb) {
        _lock_changed_cb(_locked);
    }
}

void Command::set_lock_changed_callback(Lock_Changed_Callback cb)
{
    _lock_changed_cb = cb;
}

bool Command::is_locked() const
{
    return _locked;
}

std::size_t command_t::required_arg_count() const
{
    std::size_t non_opt = 0;
    for (const auto& e : args) {
        non_opt += Command_Helpers::is_optional(e) ? 0 : 1;
    }
    return non_opt;
}
