[![CMake](https://github.com/luntar/bk_cli/actions/workflows/cmake.yml/badge.svg)](https://github.com/luntar/bk_cli/actions/workflows/cmake.yml)
# bk_cli Quick Start

`bk_cli` is a small C++17 command dispatcher. Drop it into a program when you
want a simple text command interface for debug hooks, production controls,
admin actions, service tools, or test commands.

Commands return JSON strings:

```json
{"result":"OK","msg":"running"}
```

## Build

```powershell
cmake -S . -B build
cmake --build build
.\build\Debug\bk_cli.exe
```

For single-config generators, the executable may be under `.\build\bk_cli.exe`.

The sample executable starts an interactive prompt:

```text
bk> status
{"result":"OK","msg":"running"}
```

Use the up and down arrow keys to move through command history. The line editor
also supports backspace, delete, left/right arrows, home, and end. Type `exit`
or `quit` to leave the prompt.

The GitHub Actions workflow builds the project on Windows and Ubuntu for every
push and pull request.

## Create A CLI

```cpp
#include "Command.h"

bk::Command cli;
cli.set_locked(false);
```

The built-in `help` command is registered automatically.

## Add A Simple Command

Commands are just names, help text, and a handler:

```cpp
cli.add("status", "Report system status", [](bk::args_t) {
    bk::cmd_return_t result;
    result.set_success("running");
    return result;
});

auto json = cli.execute("status", "");
```

You can also execute a full input line:

```cpp
auto json = cli.execute_line("add 2 3");
```

This is useful for production status, debug probes, admin operations, factory
tests, calibration routines, or anything else you want to expose through a
small command surface.

## Add Typed Arguments

Declare the argument types when you register the command. The CLI checks the
argument count and basic numeric format before your handler runs.

```cpp
cli.add(
    "add",
    "Add two unsigned integers",
    [](bk::args_t args) {
        const auto a = bk::Command_Helpers::to_uint32(args[0].value);
        const auto b = bk::Command_Helpers::to_uint32(args[1].value);

        bk::cmd_return_t result;
        result.set_success(std::to_string(a + b));
        return result;
    },
    { bk::argument_type::u32, bk::argument_type::u32 },
    { "a", "b" });

cli.execute("add", "2 3");
```

Common argument types:

```cpp
bk::argument_type::u32
bk::argument_type::i32
bk::argument_type::f32
bk::argument_type::str
bk::argument_type::u32_opt
bk::argument_type::str_opt
```

## Bind Variables

You can expose a variable as a get/set command. With no argument, the command
returns the current value. With one argument, it updates the value and returns
the new value.

```cpp
bool debug_enabled = false;
uint32_t sample_rate_hz = 100;

cli.bind("debug", "Get or set debug mode", debug_enabled);
cli.bind("rate", "Get or set sample rate", sample_rate_hz);

cli.execute("debug", "on");  // sets debug_enabled true
cli.execute("debug", "");    // returns true
cli.execute("rate", "250");  // sets sample_rate_hz to 250
cli.execute("rate", "");     // returns 250
```

Bool values accept:

```text
true false on off yes no 1 0
```

## Display Vectors

Vectors can be bound as read-only display commands.

```cpp
std::vector<uint32_t> adc_counts{ 1020, 1023, 1019 };

cli.bind("adc", "Display ADC counts", adc_counts);

cli.execute("adc", ""); // returns [1020, 1023, 1019]
```

This is handy for recent samples, fault history, state estimates, sensor
snapshots, or any small list you want to inspect.

## JSON Entry Point

If your command input comes from a web service or serial protocol, use
`handler()`.

```cpp
cli.set_locked(false);

auto json = cli.handler(R"({"cmd":"status","value":""})");
```

When locked, `handler()` rejects commands. Direct `execute()` calls are not
locked.

```cpp
cli.set_locked(true);
cli.handler(R"({"cmd":"status","value":""})"); // {"result":"ERROR","msg":"Locked"}
```

## Pattern

Use short command names and plain argument names:

```cpp
cli.add("reboot", "Restart the controller", reboot_handler);
cli.add("zero", "Zero the load cell", zero_handler);
cli.add("cal", "Set calibration value", cal_handler,
    { bk::argument_type::f32 },
    { "gain" });
cli.bind("armed", "Get or set armed state", armed);
cli.bind("faults", "Display fault codes", fault_codes);
```

That gives you a small command surface that is easy to type, easy to document,
and easy to call from tests or tools.

## License

MIT. See `LICENSE`.
