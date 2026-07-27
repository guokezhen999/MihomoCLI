#pragma once

#include <string>
#include <utility>

namespace clash {

class Service {
public:
    // Check if Mihomo service is running, returns {is_running, pid}
    static std::pair<bool, int> get_status();

    // Start Mihomo service silently using ~/clash/start.sh, returns {success, message}
    static std::pair<bool, std::string> start_service();

    // Stop Mihomo service silently using ~/clash/stop.sh, returns {success, message}
    static std::pair<bool, std::string> stop_service();

    // Update subscription silently using ~/clash/update.sh, returns {success, message}
    static std::pair<bool, std::string> update_subscription();

    // Show live logs via tail -n 30 -f
    static void show_live_logs();
};

} // namespace clash
