#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace bk {

class Console_Line_Editor {
public:
    explicit Console_Line_Editor(std::size_t max_history = 100);

    std::optional<std::string> read_line(const std::string& prompt);

private:
    enum class Key {
        character,
        enter,
        backspace,
        delete_key,
        left,
        right,
        up,
        down,
        home,
        end,
        eof_key,
        unknown
    };

    struct Key_Press {
        Key key{ Key::unknown };
        char value{};
    };

    void add_history(const std::string& line);
    void redraw(const std::string& prompt, const std::string& line, std::size_t cursor) const;
    Key_Press read_key() const;

    std::size_t _max_history;
    std::vector<std::string> _history;
};

} // namespace bk
