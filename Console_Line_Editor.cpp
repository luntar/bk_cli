#include "Console_Line_Editor.h"

#include <iostream>

#ifdef _WIN32
#include <conio.h>
#else
#include <cstdio>
#include <termios.h>
#include <unistd.h>
#endif

using namespace bk;

namespace {

#ifndef _WIN32
class Terminal_Raw_Mode {
public:
    Terminal_Raw_Mode()
    {
        _enabled = isatty(STDIN_FILENO) != 0 && tcgetattr(STDIN_FILENO, &_original) == 0;
        if (_enabled) {
            termios raw = _original;
            raw.c_lflag &= static_cast<unsigned>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            _enabled = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
        }
    }

    ~Terminal_Raw_Mode()
    {
        if (_enabled) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &_original);
        }
    }

    bool enabled() const
    {
        return _enabled;
    }

private:
    bool _enabled{ false };
    termios _original{};
};
#endif

bool is_printable(char c)
{
    return c >= 32 && c != 127;
}

} // namespace

Console_Line_Editor::Console_Line_Editor(std::size_t max_history)
    : _max_history(max_history)
{
}

std::optional<std::string> Console_Line_Editor::read_line(const std::string& prompt)
{
#ifndef _WIN32
    Terminal_Raw_Mode raw_mode;
    if (!raw_mode.enabled()) {
        std::cout << prompt << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            return std::nullopt;
        }
        add_history(line);
        return line;
    }
#endif

    std::string line;
    std::string draft;
    std::size_t cursor = 0;
    std::size_t history_index = _history.size();

    std::cout << prompt << std::flush;

    while (true) {
        const Key_Press key = read_key();

        switch (key.key) {
        case Key::character:
            if (is_printable(key.value)) {
                line.insert(line.begin() + static_cast<std::string::difference_type>(cursor), key.value);
                ++cursor;
                redraw(prompt, line, cursor);
            }
            break;
        case Key::enter:
            std::cout << "\n";
            add_history(line);
            return line;
        case Key::backspace:
            if (cursor > 0) {
                line.erase(cursor - 1, 1);
                --cursor;
                redraw(prompt, line, cursor);
            }
            break;
        case Key::delete_key:
            if (cursor < line.size()) {
                line.erase(cursor, 1);
                redraw(prompt, line, cursor);
            }
            break;
        case Key::left:
            if (cursor > 0) {
                --cursor;
                redraw(prompt, line, cursor);
            }
            break;
        case Key::right:
            if (cursor < line.size()) {
                ++cursor;
                redraw(prompt, line, cursor);
            }
            break;
        case Key::home:
            cursor = 0;
            redraw(prompt, line, cursor);
            break;
        case Key::end:
            cursor = line.size();
            redraw(prompt, line, cursor);
            break;
        case Key::up:
            if (!_history.empty() && history_index > 0) {
                if (history_index == _history.size()) {
                    draft = line;
                }
                --history_index;
                line = _history[history_index];
                cursor = line.size();
                redraw(prompt, line, cursor);
            }
            break;
        case Key::down:
            if (history_index < _history.size()) {
                ++history_index;
                line = (history_index == _history.size()) ? draft : _history[history_index];
                cursor = line.size();
                redraw(prompt, line, cursor);
            }
            break;
        case Key::eof_key:
            if (line.empty()) {
                std::cout << "\n";
                return std::nullopt;
            }
            break;
        case Key::unknown:
            break;
        }
    }
}

void Console_Line_Editor::add_history(const std::string& line)
{
    if (line.empty() || _max_history == 0) {
        return;
    }

    if (!_history.empty() && _history.back() == line) {
        return;
    }

    _history.push_back(line);
    if (_history.size() > _max_history) {
        _history.erase(_history.begin());
    }
}

void Console_Line_Editor::redraw(const std::string& prompt, const std::string& line, std::size_t cursor) const
{
    std::cout << "\r" << prompt << line << "\x1b[K";
    const std::size_t from_end = line.size() - cursor;
    if (from_end > 0) {
        std::cout << "\x1b[" << from_end << "D";
    }
    std::cout << std::flush;
}

Console_Line_Editor::Key_Press Console_Line_Editor::read_key() const
{
#ifdef _WIN32
    const int ch = _getch();
    if (ch == 3 || ch == 26) {
        return { Key::eof_key, 0 };
    }
    if (ch == '\r' || ch == '\n') {
        return { Key::enter, 0 };
    }
    if (ch == '\b') {
        return { Key::backspace, 0 };
    }
    if (ch == 0 || ch == 224) {
        const int extended = _getch();
        switch (extended) {
        case 71: return { Key::home, 0 };
        case 72: return { Key::up, 0 };
        case 75: return { Key::left, 0 };
        case 77: return { Key::right, 0 };
        case 79: return { Key::end, 0 };
        case 80: return { Key::down, 0 };
        case 83: return { Key::delete_key, 0 };
        default: return { Key::unknown, 0 };
        }
    }
    return { Key::character, static_cast<char>(ch) };
#else
    const int ch = std::getchar();
    if (ch == EOF || ch == 4) {
        return { Key::eof_key, 0 };
    }
    if (ch == '\r' || ch == '\n') {
        return { Key::enter, 0 };
    }
    if (ch == 127 || ch == '\b') {
        return { Key::backspace, 0 };
    }
    if (ch != '\x1b') {
        return { Key::character, static_cast<char>(ch) };
    }

    const int second = std::getchar();
    if (second != '[' && second != 'O') {
        return { Key::unknown, 0 };
    }

    const int third = std::getchar();
    switch (third) {
    case 'A': return { Key::up, 0 };
    case 'B': return { Key::down, 0 };
    case 'C': return { Key::right, 0 };
    case 'D': return { Key::left, 0 };
    case 'F': return { Key::end, 0 };
    case 'H': return { Key::home, 0 };
    case '3':
        if (std::getchar() == '~') {
            return { Key::delete_key, 0 };
        }
        return { Key::unknown, 0 };
    default:
        return { Key::unknown, 0 };
    }
#endif
}
