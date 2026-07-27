#include "clash-cli/api.hpp"
#include "clash-cli/config.hpp"
#include "httplib/httplib.h"
#include <regex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace clash {

APIClient::APIClient(int ctrl_port, const std::string& secret)
    : ctrl_port_(ctrl_port), secret_(secret) {}

std::optional<nlohmann::json> APIClient::make_request(const std::string& method, const std::string& path, const nlohmann::json& body) {
    httplib::Client cli("127.0.0.1", ctrl_port_);
    cli.set_connection_timeout(2, 0); // 2s
    cli.set_read_timeout(2, 0);       // 2s

    httplib::Headers headers;
    if (!secret_.empty()) {
        headers.insert({"Authorization", "Bearer " + secret_});
    }

    httplib::Result res;
    if (method == "GET") {
        res = cli.Get(path, headers);
    } else if (method == "PUT") {
        std::string body_str = (body != nullptr) ? body.dump() : "";
        res = cli.Put(path, headers, body_str, "application/json");
    } else if (method == "PATCH") {
        std::string body_str = (body != nullptr) ? body.dump() : "";
        res = cli.Patch(path, headers, body_str, "application/json");
    } else if (method == "POST") {
        std::string body_str = (body != nullptr) ? body.dump() : "";
        res = cli.Post(path, headers, body_str, "application/json");
    } else {
        return std::nullopt;
    }

    if (res && (res->status == 200 || res->status == 201 || res->status == 204)) {
        if (res->body.empty()) {
            return nlohmann::json::object();
        }
        try {
            return nlohmann::json::parse(res->body);
        } catch (...) {
            return nlohmann::json::object();
        }
    }
    return std::nullopt;
}

std::optional<std::string> APIClient::get_mode() {
    auto res = make_request("GET", "/configs");
    if (res && res->contains("mode")) {
        return (*res)["mode"].get<std::string>();
    }
    return std::nullopt;
}

bool APIClient::set_mode(const std::string& mode) {
    std::string target_mode = "";
    std::string m = mode;
    std::transform(m.begin(), m.end(), m.begin(), ::tolower);
    if (m == "rule") target_mode = "Rule";
    else if (m == "global") target_mode = "Global";
    else if (m == "direct") target_mode = "Direct";
    else return false;

    auto res = make_request("PATCH", "/configs", {{"mode", target_mode}});
    return res.has_value();
}

std::vector<std::pair<std::string, APIClient::SelectorInfo>> APIClient::get_selectors() {
    std::vector<std::pair<std::string, SelectorInfo>> result;
    auto res = make_request("GET", "/proxies");
    if (!res || !res->contains("proxies")) {
        return result;
    }

    auto proxies = (*res)["proxies"];
    for (auto& [name, info] : proxies.items()) {
        if (info.contains("type") && info["type"] == "Selector") {
            SelectorInfo sel;
            sel.now = info.value("now", "");
            if (info.contains("all") && info["all"].is_array()) {
                for (auto& item : info["all"]) {
                    sel.all.push_back(item.get<std::string>());
                }
            }
            result.push_back({name, sel});
        }
    }
    return result;
}

// URL encode helper
static std::string url_encode(const std::string& val) {
    std::ostringstream escaped;
    escaped << std::hex;
    for (char c : val) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
        }
    }
    return escaped.str();
}

bool APIClient::switch_node(const std::string& group_name, const std::string& node_name) {
    std::string encoded_group = url_encode(group_name);
    auto res = make_request("PUT", "/proxies/" + encoded_group, {{"name", node_name}});
    if (res.has_value()) {
        if (group_name == "三毛机场" || group_name == "GLOBAL") {
            try {
                auto pref_file = Config::get_home_directory() / "clash" / "preferred_node";
                std::ofstream f(pref_file);
                if (f.is_open()) {
                    f << node_name;
                }
            } catch (...) {}
        }
        return true;
    }
    return false;
}

// IP Lookup Direct
IPInfo APIClient::get_ip_info_direct() {
    IPInfo info;
    httplib::Client cli("http://ip-api.com");
    cli.set_connection_timeout(3, 0);
    cli.set_read_timeout(3, 0);

    auto res = cli.Get("/json/?lang=zh-CN");
    if (res && res->status == 200) {
        try {
            auto j = nlohmann::json::parse(res->body);
            if (j.value("status", "") == "success") {
                info.ip = j.value("query", "");
                info.country = j.value("country", "");
                info.region = j.value("regionName", "");
                info.city = j.value("city", "");
                info.isp = j.value("isp", "");
                info.success = true;
                return info;
            }
        } catch (...) {}
    }

    // Fallback to myip.ipip.net
    httplib::Client cli_fallback("http://myip.ipip.net");
    cli_fallback.set_connection_timeout(3, 0);
    cli_fallback.set_read_timeout(3, 0);
    
    httplib::Headers headers = {
        {"User-Agent", "Mozilla/5.0"}
    };
    auto res_fb = cli_fallback.Get("/", headers);
    if (res_fb && res_fb->status == 200) {
        std::string body = res_fb->body;
        std::regex re("当前 IP：([^\\s]+)\\s+来自于：(.*)");
        std::smatch m;
        if (std::regex_search(body, m, re) && m.size() >= 3) {
            info.ip = m[1].str();
            info.country = m[2].str();
            info.success = true;
            return info;
        }
    }
    return info;
}

// IP Lookup via Proxy
IPInfo APIClient::get_ip_info_proxy(int proxy_port) {
    IPInfo info;
    httplib::Client cli("http://ip-api.com");
    cli.set_connection_timeout(3, 0);
    cli.set_read_timeout(3, 0);
    cli.set_proxy("127.0.0.1", proxy_port);

    auto res = cli.Get("/json/?lang=zh-CN");
    if (res && res->status == 200) {
        try {
            auto j = nlohmann::json::parse(res->body);
            if (j.value("status", "") == "success") {
                info.ip = j.value("query", "");
                info.country = j.value("country", "");
                info.region = j.value("regionName", "");
                info.city = j.value("city", "");
                info.isp = j.value("isp", "");
                info.success = true;
                return info;
            }
        } catch (...) {}
    }
    return info;
}

} // namespace clash
