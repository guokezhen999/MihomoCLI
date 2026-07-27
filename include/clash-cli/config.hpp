#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace clash {

struct Subscription {
    std::string name;
    std::string url;
};

class SubscriptionManager {
public:
    static std::filesystem::path get_json_path();
    static std::filesystem::path get_url_path();
    
    // Load subscription list, populates active_url. Migrates if needed.
    static std::vector<Subscription> load(std::string& active_url);
    
    // Save subscription list and update subscription.url with the active one.
    static bool save(const std::vector<Subscription>& list, const std::string& active_url);
};

struct Config {
    int ctrl_port = 19090;
    int mixed_port = 17890;
    std::string secret = "";

    // Load configuration from ~/clash/config/config.yaml
    bool load();
    
    // Get home directory path helper
    static std::filesystem::path get_home_directory();
};

} // namespace clash
