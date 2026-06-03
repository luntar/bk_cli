#include "Command.h"
#include "Console_Line_Editor.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool g_enabled{ false };
uint32_t g_count{ 10 };
std::vector<uint32_t> g_samples{ 10, 20, 30, 40 };

} // namespace

int main()
{
    bk::Command cli;
    cli.set_locked(false);

    cli.add(
        "echo",
        "Echo one string argument",
        [](bk::args_t args) {
            bk::cmd_return_t result;
            result.set_success(args.front().value);
            return result;
        },
        { bk::argument_type::str },
        { "text" });

    cli.add("status", "Report that the CLI is running", [](bk::args_t) {
        bk::cmd_return_t result;
        result.set_success("running");
        return result;
    });

    cli.add(
        "add",
        "Add two unsigned integers",
        [](bk::args_t args) {
            const auto a = bk::Command_Helpers::to_uint32(args[0].value, 0);
            const auto b = bk::Command_Helpers::to_uint32(args[1].value, 0);

            bk::cmd_return_t result;
            result.set_success(std::to_string(a + b));
            return result;
        },
        { bk::argument_type::u32, bk::argument_type::u32 },
        { "a", "b" });

    cli.bind("enabled", "Get or set the enabled flag", g_enabled);
    cli.bind("count", "Get or set the count value", g_count);
    cli.bind("samples", "Display sample values", g_samples);

    bk::Console_Line_Editor editor;

    std::cout << "Type 'help' for commands or 'exit' to quit.\n";
    while (auto line = editor.read_line("bk> ")) {
        if (*line == "exit" || *line == "quit") {
            break;
        }

        if (line->find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        std::cout << cli.execute_line(*line) << '\n';
    }

    return 0;
}
