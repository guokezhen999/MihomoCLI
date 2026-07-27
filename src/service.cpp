#include "clash-cli/service.hpp"
#include "clash-cli/config.hpp"
#include "clash-cli/tui.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <array>
#include <memory>
#include <filesystem>

namespace clash {

// Helper to run a command and capture exit code and output
static std::pair<int, std::string> run_cmd_capture(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::string cmd_with_err = cmd + " 2>&1";
    FILE* pipe = popen(cmd_with_err.c_str(), "r");
    if (!pipe) {
        return {-1, "Failed to run command: popen failed"};
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    int status = pclose(pipe);
    int exit_code = WEXITSTATUS(status);
    return {exit_code, result};
}

std::pair<bool, int> Service::get_status() {
    auto home = Config::get_home_directory();
    auto pid_file = home / "clash" / "mihomo.pid";
    if (std::filesystem::exists(pid_file)) {
        try {
            std::ifstream f(pid_file);
            int pid;
            if (f >> pid) {
                if (kill(pid, 0) == 0) {
                    return {true, pid};
                }
            }
        } catch (...) {}
    }

    auto [exit_code, out] = run_cmd_capture("pgrep -f \"mihomo -d\"");
    if (exit_code == 0 && !out.empty()) {
        std::stringstream ss(out);
        int pid;
        if (ss >> pid) {
            return {true, pid};
        }
    }
    return {false, 0};
}

std::pair<bool, std::string> Service::start_service() {
    auto home = Config::get_home_directory();
    auto script = home / "clash" / "start.sh";
    if (!std::filesystem::exists(script)) {
        return {false, "start.sh script not found"};
    }
    auto [exit_code, out] = run_cmd_capture(script.string());
    if (exit_code == 0) {
        return {true, "Service started successfully."};
    }
    if (out.empty()) out = "Exit code: " + std::to_string(exit_code);
    return {false, out};
}

std::pair<bool, std::string> Service::stop_service() {
    auto home = Config::get_home_directory();
    auto script = home / "clash" / "stop.sh";
    if (!std::filesystem::exists(script)) {
        return {false, "stop.sh script not found"};
    }
    auto [exit_code, out] = run_cmd_capture(script.string());
    if (exit_code == 0) {
        return {true, "Service stopped successfully."};
    }
    if (out.empty()) out = "Exit code: " + std::to_string(exit_code);
    return {false, out};
}

std::pair<bool, std::string> Service::update_subscription() {
    auto home = Config::get_home_directory();
    auto script = home / "clash" / "update.sh";
    if (!std::filesystem::exists(script)) {
        return {false, "update.sh script not found"};
    }
    auto [exit_code, out] = run_cmd_capture(script.string());
    if (exit_code == 0) {
        return {true, "Subscription updated successfully."};
    }
    if (out.empty()) out = "Exit code: " + std::to_string(exit_code);
    return {false, out};
}

void Service::show_live_logs() {
    auto home = Config::get_home_directory();
    auto log_file = home / "clash" / "logs" / "mihomo.log";
    if (!std::filesystem::exists(log_file)) {
        std::cout << color::RED << "✖ Log file not found at " << log_file << color::RESET << "\n";
        return;
    }
    std::string cmd = "tail -n 30 -f " + log_file.string();
    std::system(cmd.c_str());
}

} // namespace clash
