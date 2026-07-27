#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <optional>

namespace clash {

// Terminal Colors
namespace color {
    const std::string GREEN = "\033[32m";
    const std::string RED = "\033[31m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string CYAN = "\033[36m";
    const std::string BOLD = "\033[1m";
    const std::string RESET = "\033[0m";
}

// Keyboard input keys
enum class Key {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    ENTER,
    ESC,
    BACKSPACE,
    CTRL_C,
    CHAR,
    UNKNOWN
};

struct KeyInput {
    Key type;
    char ch = 0;
};

// Retrieve raw single keyboard keystroke
KeyInput get_key();

// Read input string interactively by restoring canonical mode temporarily
std::string read_input(const std::string& prompt);

// Spinner class to show loading animation in background
class Spinner {
public:
    Spinner(const std::string& message = "Loading...");
    ~Spinner();
    void stop();

private:
    std::string message_;
    std::atomic<bool> stop_;
    std::thread thread_;
};

// Calculate display visual width of strings, accounting for UTF-8 characters
int get_display_width(const std::string& s);

// Pad a string to target visual width
std::string pad_string(const std::string& s, int width);

// Generate side-by-side string representation of a list
std::string print_in_columns_string(const std::vector<std::string>& items, int cols = 2, int col_width = 42);

// Interactive arrow key list selector. Returns selected index or std::nullopt on cancel.
std::optional<int> select_from_list(const std::string& title, const std::vector<std::string>& items, int default_idx = 0);

// Interactive list selector with real-time fuzzy text search filtering. Returns node name or std::nullopt.
std::optional<std::string> select_from_list_with_search(const std::string& title, const std::vector<std::string>& items, int default_idx = 0);

} // namespace clash
