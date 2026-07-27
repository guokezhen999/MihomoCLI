#include "clash-cli/config.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace clash {

std::filesystem::path Config::get_home_directory() {
    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home);
    }
    struct passwd* pw = getpwuid(getuid());
    if (pw) {
        return std::filesystem::path(pw->pw_dir);
    }
    return std::filesystem::path("/");
}

bool Config::load() {
    std::filesystem::path config_path = get_home_directory() / "clash" / "config" / "config.yaml";
    if (!std::filesystem::exists(config_path)) {
        return false;
    }
    
    std::ifstream f(config_path);
    if (!f.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(f, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line.empty() || line[0] == '#') continue;
        
        if (line.rfind("external-controller:", 0) == 0) {
            std::string val = line.substr(20);
            size_t vstart = val.find_first_not_of(" \t'\"");
            size_t vend = val.find_last_not_of(" \t'\"");
            if (vstart != std::string::npos && vend != std::string::npos) {
                val = val.substr(vstart, vend - vstart + 1);
            }
            size_t colon = val.find_last_of(':');
            if (colon != std::string::npos) {
                try {
                    ctrl_port = std::stoi(val.substr(colon + 1));
                } catch (...) {}
            } else {
                try {
                    ctrl_port = std::stoi(val);
                } catch (...) {}
            }
        } else if (line.rfind("mixed-port:", 0) == 0) {
            std::string val = line.substr(11);
            size_t vstart = val.find_first_not_of(" \t'\"");
            size_t vend = val.find_last_not_of(" \t'\"");
            if (vstart != std::string::npos && vend != std::string::npos) {
                val = val.substr(vstart, vend - vstart + 1);
            }
            try {
                mixed_port = std::stoi(val);
            } catch (...) {}
        } else if (line.rfind("secret:", 0) == 0) {
            std::string val = line.substr(7);
            size_t vstart = val.find_first_not_of(" \t'\"");
            size_t vend = val.find_last_not_of(" \t'\"");
            if (vstart != std::string::npos && vend != std::string::npos) {
                secret = val.substr(vstart, vend - vstart + 1);
            } else {
                secret = "";
            }
        }
    }
    return true;
}

std::filesystem::path SubscriptionManager::get_json_path() {
    return Config::get_home_directory() / "clash" / "config" / "subscriptions.json";
}

std::filesystem::path SubscriptionManager::get_url_path() {
    return Config::get_home_directory() / "clash" / "config" / "subscription.url";
}

std::vector<Subscription> SubscriptionManager::load(std::string& active_url) {
    std::vector<Subscription> list;
    auto json_path = get_json_path();
    auto url_path = get_url_path();
    
    if (!std::filesystem::exists(json_path)) {
        // Migrate from subscription.url if possible
        std::string current_url = "";
        if (std::filesystem::exists(url_path)) {
            std::ifstream uf(url_path);
            if (uf.is_open()) {
                std::getline(uf, current_url);
                // Trim trailing \r or whitespace
                while (!current_url.empty() && (current_url.back() == '\r' || current_url.back() == '\n' || current_url.back() == ' ')) {
                    current_url.pop_back();
                }
            }
        }
        
        if (!current_url.empty()) {
            Subscription sub;
            sub.name = "三毛机场"; // Default name from system
            sub.url = current_url;
            list.push_back(sub);
            active_url = current_url;
            save(list, active_url);
        } else {
            active_url = "";
        }
        return list;
    }
    
    try {
        std::ifstream jf(json_path);
        if (jf.is_open()) {
            nlohmann::json j;
            jf >> j;
            active_url = j.value("active", "");
            if (j.contains("list") && j["list"].is_array()) {
                for (auto& item : j["list"]) {
                    Subscription sub;
                    sub.name = item.value("name", "");
                    sub.url = item.value("url", "");
                    list.push_back(sub);
                }
            }
        }
    } catch (...) {}
    
    return list;
}

bool SubscriptionManager::save(const std::vector<Subscription>& list, const std::string& active_url) {
    auto json_path = get_json_path();
    auto url_path = get_url_path();
    
    try {
        std::filesystem::create_directories(json_path.parent_path());
        
        // Write JSON
        nlohmann::json j;
        j["active"] = active_url;
        nlohmann::json arr = nlohmann::json::array();
        for (auto& sub : list) {
            nlohmann::json item;
            item["name"] = sub.name;
            item["url"] = sub.url;
            arr.push_back(item);
        }
        j["list"] = arr;
        
        std::ofstream jf(json_path);
        if (jf.is_open()) {
            jf << j.dump(4);
        } else {
            return false;
        }
        
        // Write raw URL to subscription.url for update.sh compatibility
        std::ofstream uf(url_path);
        if (uf.is_open()) {
            uf << active_url << "\n";
        } else {
            return false;
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace clash
