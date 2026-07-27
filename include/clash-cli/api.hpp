#pragma once

#include <string>
#include <vector>
#include <optional>
#include "nlohmann/json.hpp"

namespace clash {

struct IPInfo {
    std::string ip;
    std::string country;
    std::string region;
    std::string city;
    std::string isp;
    bool success = false;
};

class APIClient {
public:
    APIClient(int ctrl_port, const std::string& secret);

    // Make an API request to Clash/Mihomo controller. Returns JSON response or nullopt on failure.
    std::optional<nlohmann::json> make_request(const std::string& method, const std::string& path, const nlohmann::json& body = nullptr);

    // Retrieve active routing mode
    std::optional<std::string> get_mode();

    // Update active routing mode
    bool set_mode(const std::string& mode);

    // Retrieve selectors. Format is a map from group name to structure containing current selection and options list.
    struct SelectorInfo {
        std::string now;
        std::vector<std::string> all;
    };
    std::vector<std::pair<std::string, SelectorInfo>> get_selectors();

    // Switch selected node for a proxy group
    bool switch_node(const std::string& group_name, const std::string& node_name);

    // Retrieve IP information (Direct)
    static IPInfo get_ip_info_direct();

    // Retrieve IP information (Via proxy port)
    static IPInfo get_ip_info_proxy(int proxy_port);

private:
    int ctrl_port_;
    std::string secret_;
};

} // namespace clash
