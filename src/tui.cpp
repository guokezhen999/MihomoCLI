#include "clash-cli/tui.hpp"
#include <iostream>
#include <algorithm>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <chrono>

namespace clash {

KeyInput get_key() {
    KeyInput result;
    result.type = Key::UNKNOWN;

    struct termios old_settings, new_settings;
    if (tcgetattr(STDIN_FILENO, &old_settings) < 0) {
        return result;
    }

    new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) < 0) {
        return result;
    }

    char buf[8];
    ssize_t n = read(STDIN_FILENO, buf, 1);
    if (n <= 0) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
        return result;
    }

    char ch = buf[0];
    if (ch == '\x1b') {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000; // 50ms timeout for escape sequences

        int sel = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
        if (sel > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
            char seq[3];
            ssize_t seq_n = read(STDIN_FILENO, seq, 2);
            if (seq_n == 2) {
                if (seq[0] == '[') {
                    if (seq[1] == 'A') result.type = Key::UP;
                    else if (seq[1] == 'B') result.type = Key::DOWN;
                    else if (seq[1] == 'C') result.type = Key::RIGHT;
                    else if (seq[1] == 'D') result.type = Key::LEFT;
                }
            }
        } else {
            result.type = Key::ESC;
        }
    } else if (ch == '\n' || ch == '\r') {
        result.type = Key::ENTER;
    } else if (ch == '\x03') {
        result.type = Key::CTRL_C;
    } else if (ch == '\x7f' || ch == '\x08') {
        result.type = Key::BACKSPACE;
    } else {
        result.type = Key::CHAR;
        result.ch = ch;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
    return result;
}

Spinner::Spinner(const std::string& message)
    : message_(message), stop_(false) {
    thread_ = std::thread([this]() {
        std::vector<std::string> frames = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
        size_t idx = 0;
        std::cout << "\033[?25l" << std::flush;
        while (!stop_) {
            std::cout << "\r" << color::CYAN << frames[idx] << " " << color::RESET << message_ << std::flush;
            idx = (idx + 1) % frames.size();
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
        std::cout << "\r\033[K" << std::flush;
        std::cout << "\033[?25h" << std::flush;
    });
}

Spinner::~Spinner() {
    stop();
}

void Spinner::stop() {
    if (!stop_) {
        stop_ = true;
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

int get_display_width(const std::string& s) {
    int width = 0;
    for (size_t i = 0; i < s.length(); ) {
        unsigned char c = s[i];
        if (c < 0x80) {
            width += 1;
            i += 1;
        } else if ((c & 0xe0) == 0xc0) {
            width += 2;
            i += 2;
        } else if ((c & 0xf0) == 0xe0) {
            width += 2;
            i += 3;
        } else if ((c & 0xf8) == 0xf0) {
            width += 2;
            i += 4;
        } else {
            i += 1;
        }
    }
    return width;
}

std::string pad_string(const std::string& s, int width) {
    int dw = get_display_width(s);
    if (dw >= width) {
        return s;
    }
    return s + std::string(width - dw, ' ');
}

std::string print_in_columns_string(const std::vector<std::string>& items, int cols, int col_width) {
    std::string out = "";
    int n_items = items.size();
    for (int i = 0; i < n_items; i += cols) {
        std::string row_str = "";
        for (int idx = 0; idx < cols && (i + idx) < n_items; ++idx) {
            int actual_idx = i + idx + 1;
            char buf[32];
            snprintf(buf, sizeof(buf), "%2d) ", actual_idx);
            std::string prefix(buf);
            
            std::string item_text = items[i + idx];
            std::string col_text = prefix + item_text;
            int dw = get_display_width(col_text);
            
            std::string padded = col_text;
            if (dw < col_width) {
                padded += std::string(col_width - dw, ' ');
            }
            
            std::string colored_prefix = color::GREEN + prefix + color::RESET;
            padded.replace(0, prefix.length(), colored_prefix);
            
            row_str += padded;
        }
        if (i > 0) out += "\n";
        out += row_str;
    }
    return out;
}

std::optional<int> select_from_list(const std::string& title, const std::vector<std::string>& items, int default_idx) {
    std::cout << "\033[?25l" << std::flush;
    
    int idx = default_idx;
    int n_items = items.size();
    int max_visible = 12;
    int start_idx = 0;
    
    try {
        while (true) {
            if (idx < start_idx) {
                start_idx = idx;
            } else if (idx >= start_idx + max_visible) {
                start_idx = idx - max_visible + 1;
            }
            
            int lines_written = 0;
            std::cout << "\n" << color::BOLD << title << color::RESET << "\n";
            lines_written += 2;
            
            for (int i = start_idx; i < std::min(start_idx + max_visible, n_items); ++i) {
                if (i == idx) {
                    std::cout << "  " << color::GREEN << "❯ " << items[i] << color::RESET << "\n";
                } else {
                    std::cout << "    " << items[i] << "\n";
                }
                lines_written++;
            }
            
            if (n_items > max_visible) {
                std::cout << "  " << color::CYAN << "─── (第 " << (idx + 1) << "/" << n_items << " 项, 用 ↑↓ 移动, Enter 确定) ───" << color::RESET << "\n";
                lines_written++;
            } else {
                std::cout << "  " << color::CYAN << "─── (用 ↑↓ 移动, Enter 确定) ───" << color::RESET << "\n";
                lines_written++;
            }
            std::cout << std::flush;
            
            KeyInput key = get_key();
            
            std::cout << "\033[" << lines_written << "A\033[J" << std::flush;
            
            if (key.type == Key::UP) {
                idx = (idx - 1 + n_items) % n_items;
            } else if (key.type == Key::DOWN) {
                idx = (idx + 1) % n_items;
            } else if (key.type == Key::ENTER) {
                std::cout << "\033[?25h" << std::flush;
                return idx;
            } else if (key.type == Key::ESC || key.type == Key::CTRL_C) {
                std::cout << "\033[?25h" << std::flush;
                return std::nullopt;
            }
        }
    } catch (...) {
        std::cout << "\033[?25h" << std::flush;
        throw;
    }
}

std::optional<std::string> select_from_list_with_search(const std::string& title, const std::vector<std::string>& items, int default_idx) {
    std::cout << "\033[?25l" << std::flush;
    
    int idx = default_idx;
    std::string query = "";
    int n_all = items.size();
    
    try {
        while (true) {
            std::vector<std::pair<int, std::string>> filtered_items;
            if (!query.empty()) {
                std::string lower_query = query;
                std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
                for (int i = 0; i < n_all; ++i) {
                    std::string lower_item = items[i];
                    std::transform(lower_item.begin(), lower_item.end(), lower_item.begin(), ::tolower);
                    if (lower_item.find(lower_query) != std::string::npos) {
                        filtered_items.push_back({i, items[i]});
                    }
                }
            } else {
                for (int i = 0; i < n_all; ++i) {
                    filtered_items.push_back({i, items[i]});
                }
            }
            
            int n_items = filtered_items.size();
            
            int lines_written = 0;
            std::cout << "\n" << color::BOLD << title << color::RESET << "\n";
            if (!query.empty()) {
                std::cout << "  " << color::CYAN << "🔍 搜索过滤: " << color::BOLD << color::GREEN << query << color::RESET 
                          << " (匹配到 " << n_items << " 个节点, 退格键清除)\n";
            } else {
                std::cout << "  " << color::CYAN << "⌨️ 直接输入字母搜索 (用 ↑↓ 移动, Enter 确定)\n";
            }
            lines_written += 2;
            
            int max_visible = 10;
            int start_idx = 0;
            
            if (n_items > 0) {
                if (idx >= n_items) idx = n_items - 1;
                if (idx < 0) idx = 0;
                
                if (idx < start_idx) {
                    start_idx = idx;
                } else if (idx >= start_idx + max_visible) {
                    start_idx = idx - max_visible + 1;
                }
                
                for (int i = start_idx; i < std::min(start_idx + max_visible, n_items); ++i) {
                    if (i == idx) {
                        std::cout << "  " << color::GREEN << "❯ " << filtered_items[i].second << color::RESET << "\n";
                    } else {
                        std::cout << "    " << filtered_items[i].second << "\n";
                    }
                    lines_written++;
                }
            } else {
                std::cout << "    " << color::RED << "没有匹配的节点..." << color::RESET << "\n";
                lines_written++;
            }
            
            if (n_items > max_visible) {
                std::cout << "  " << color::CYAN << "─── (第 " << (idx + 1) << "/" << n_items << " 项) ───" << color::RESET << "\n";
                lines_written++;
            } else {
                std::cout << "  " << color::CYAN << "──────────────────────────────" << color::RESET << "\n";
                lines_written++;
            }
            std::cout << std::flush;
            
            KeyInput key = get_key();
            
            std::cout << "\033[" << lines_written << "A\033[J" << std::flush;
            
            if (key.type == Key::UP) {
                if (n_items > 0) {
                    idx = (idx - 1 + n_items) % n_items;
                }
            } else if (key.type == Key::DOWN) {
                if (n_items > 0) {
                    idx = (idx + 1) % n_items;
                }
            } else if (key.type == Key::ENTER) {
                if (n_items > 0) {
                    std::cout << "\033[?25h" << std::flush;
                    return filtered_items[idx].second;
                }
                std::cout << "\033[?25h" << std::flush;
                return std::nullopt;
            } else if (key.type == Key::ESC || key.type == Key::CTRL_C) {
                std::cout << "\033[?25h" << std::flush;
                return std::nullopt;
            } else if (key.type == Key::BACKSPACE) {
                if (!query.empty()) {
                    query.pop_back();
                    idx = 0;
                }
            } else if (key.type == Key::CHAR) {
                if (key.ch >= 32 && key.ch <= 126) {
                    query += key.ch;
                    idx = 0;
                }
            }
        }
    } catch (...) {
        std::cout << "\033[?25h" << std::flush;
        throw;
    }
}

std::string read_input(const std::string& prompt) {
    struct termios old_settings;
    if (tcgetattr(STDIN_FILENO, &old_settings) < 0) {
        return "";
    }

    struct termios new_settings = old_settings;
    new_settings.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);

    std::cout << prompt << std::flush;
    std::string input;
    std::getline(std::cin, input);

    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
    return input;
}

} // namespace clash
