#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Command_Helpers.h"

namespace bk {

namespace detail {

template <typename T>
std::string value_to_string(const T& value)
{
    if constexpr (std::is_same_v<T, bool>) {
        return value ? "true" : "false";
    }
    else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>) {
        return std::to_string(static_cast<int>(value));
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        return value;
    }
    else {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
}

template <typename T>
std::string vector_to_string(const std::vector<T>& values)
{
    std::string out = "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += value_to_string(values[i]);
    }
    out += "]";
    return out;
}

} // namespace detail

using Lock_Changed_Callback = std::function<void(bool locked)>;

class cmd_return_t {
    static constexpr const char* k_success_str = "OK";
    static constexpr const char* k_fail_str = "ERROR";

public:
    cmd_return_t(bool success = true, std::string msg = k_success_str) {
        success ? set_success(msg) : set_error(msg);
    }

    void set_error(const std::string& msg = k_fail_str) {
        _success = false;
        _msg = msg;
    }

    void set_success(const std::string& msg = k_success_str) {
        _success = true;
        _msg = msg;
    }

    std::string to_string() const {
        return make_result_str(_success ? k_success_str : k_fail_str, _msg);
    }

    void set_success() {
        _success = true;
        _msg = k_success_str;
    }

    void set_error() {
        _success = false;
        _msg = k_fail_str;
    }

    bool is_success() const {
        return _success;
    }

private:
    static std::string make_result_str(const std::string& result, const std::string& msg);

    bool _success{ true };
    std::string _msg{ k_success_str };
};

using arg_def_t = std::vector<argument_type>;

struct arg_t {
    argument_type type{ argument_type::unknown };
    std::string value{};
};

using args_t = std::vector<arg_t>;

using handler_func = std::function<cmd_return_t(args_t)>;

struct command_t {
    std::string name{};
    arg_def_t args{};
    std::vector<std::string> args_doc;
    std::string doc{};
    handler_func h{};
    std::string parent{};
    std::vector<command_t*> children;

    void reset() {
        name.clear();
        args.clear();
        args_doc.clear();
        doc.clear();
        h = nullptr;
        parent.clear();
        children.clear();
    }

    std::string get_arg_doc(std::size_t idx) const noexcept {
        if (idx < args_doc.size()) {
            return args_doc[idx];
        }
        return (idx < args.size()) ? Command_Helpers::argument_type_to_string(args[idx]) : "";
    }

    void set_parent(command_t& p) {
        parent = p.name;
        p.children.push_back(this);
    }

    bool is_parent() const {
        return !children.empty();
    }

    bool is_child() const {
        return !parent.empty();
    }

    bool is_valid() const {
        return !name.empty();
    }

    std::size_t required_arg_count() const;
};

/**
 * @brief A class for managing CLI commands
 *
 * Register commands with add(), then call execute() with the command name
 * and whitespace-separated argument string.
 */
class Command {
    static constexpr std::size_t k_help_name_width = 30;

public:
    Command();

    /** Add a fully specified command. */
    void add(command_t cmd) noexcept;

    /** Add a command without manually constructing command_t. */
    void add(
        std::string name,
        std::string doc,
        handler_func h,
        arg_def_t args = {},
        std::vector<std::string> args_doc = {}) noexcept;

    /** Add a command that gets or sets a referenced bool. */
    void bind(std::string name, std::string doc, bool& value) noexcept;

    /** Add a command that gets or sets a referenced uint32_t. */
    void bind(std::string name, std::string doc, uint32_t& value) noexcept;

    /** Add a read-only command that displays a referenced vector. */
    template <typename T>
    void bind(std::string name, std::string doc, const std::vector<T>& values) noexcept
    {
        add(
            std::move(name),
            std::move(doc),
            [&values](args_t) {
                cmd_return_t result;
                result.set_success(detail::vector_to_string(values));
                return result;
            });
    }

    void add_child(command_t& parent, command_t subcmd) noexcept;

    std::string build_sub_cmd_name(std::string n, const std::string& accumulated = "") const;

    /** Return a registered command, or an invalid command_t when not found. */
    command_t get(std::string n) const noexcept;

    /** Return a pointer to a registered command, or nullptr when not found. */
    command_t* get_ptr(const std::string& n) noexcept;

    /** Return the command documentation string, or an empty string when not found. */
    std::string get_doc(std::string n) const noexcept;

    /** Execute a registered command using a whitespace-separated argument string. */
    std::string execute(const std::string& cmd, const std::string& args);

    /**
     * Execute a command from JSON:
     * {"cmd":"some_cmd_name","value":"1 foo 1234 a"}
     */
    std::string handler(const std::string& cmd_str);

    /** True when the JSON handler rejects commands. */
    bool is_locked() const;

    /** Set whether the JSON handler rejects commands. */
    void set_locked(bool locked);

    /** Set the callback fired when the lock state changes. */
    void set_lock_changed_callback(Lock_Changed_Callback cb);

private:
    std::map<std::string, command_t> _commands{};

    args_t make_args_from_string(const std::string& args, const arg_def_t& def);

    void take_first_arg(std::string& args);
    std::string first_arg(const std::string& args);

    bool _locked{ true };

    Lock_Changed_Callback _lock_changed_cb;
};

} // namespace bk
