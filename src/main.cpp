#include "clash-cli/config.hpp"
#include "clash-cli/api.hpp"
#include "clash-cli/tui.hpp"
#include "clash-cli/service.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace clash;

void print_help() {
    std::cout << color::BOLD << "Mihomo (Clash Meta) CLI Manager" << color::RESET << "\n\n"
              << "Usage:\n"
              << "  clash-cli                 Open the interactive keyboard dashboard (default)\n"
              << "  clash-cli start           Start the Mihomo proxy service\n"
              << "  clash-cli stop            Stop the Mihomo proxy service\n"
              << "  clash-cli restart         Restart the Mihomo proxy service\n"
              << "  clash-cli status          Show the current status of the service\n"
              << "  clash-cli mode [name]     Get or set the routing mode (rule/global/direct)\n"
              << "  clash-cli select [query]  Fuzzy search and select a node, or switch to match\n"
              << "  clash-cli update          Update subscription configurations\n"
              << "  clash-cli ip              Query current outbound IP address and geographic location\n"
              << "  clash-cli log             View and follow the live logs\n"
              << "  clash-cli help            Show this help message\n\n";
}

void run_ip_cli(int mixed_port) {
    IPInfo local_info, proxy_info;
    {
        Spinner s("Fetching IP information (Direct & Proxy)...");
        local_info = APIClient::get_ip_info_direct();
        proxy_info = APIClient::get_ip_info_proxy(mixed_port);
    }
    std::cout << color::GREEN << "✔ IP information retrieved successfully:" << color::RESET << "\n\n";
    if (local_info.success) {
        std::vector<std::string> parts = {local_info.country, local_info.region, local_info.city};
        std::string loc = "";
        for (auto& p : parts) if (!p.empty()) loc += p + " ";
        std::cout << "  " << color::BOLD << "本地 IP (Direct):" << color::RESET << "\n"
                  << "    IP Address: " << color::GREEN << local_info.ip << color::RESET << "\n"
                  << "    Location:   " << color::YELLOW << loc << color::RESET << "\n";
        if (!local_info.isp.empty()) {
            std::cout << "    ISP:        " << color::CYAN << local_info.isp << color::RESET << "\n";
        }
    } else {
        std::cout << "  " << color::BOLD << "本地 IP (Direct):" << color::RESET << " " << color::RED << "Failed to query" << color::RESET << "\n";
    }
    std::cout << "\n";
    if (proxy_info.success) {
        std::vector<std::string> parts = {proxy_info.country, proxy_info.region, proxy_info.city};
        std::string loc = "";
        for (auto& p : parts) if (!p.empty()) loc += p + " ";
        std::cout << "  " << color::BOLD << "代理 IP (Proxy):" << color::RESET << "\n"
                  << "    IP Address: " << color::GREEN << proxy_info.ip << color::RESET << "\n"
                  << "    Location:   " << color::YELLOW << loc << color::RESET << "\n";
        if (!proxy_info.isp.empty()) {
            std::cout << "    ISP:        " << color::CYAN << proxy_info.isp << color::RESET << "\n";
        }
    } else {
        std::cout << "  " << color::BOLD << "代理 IP (Proxy):" << color::RESET << " " << color::RED << "Failed to query (Is Mihomo running?)" << color::RESET << "\n";
    }
}

void check_ip_command_with_spinner(int mixed_port) {
    IPInfo local_info, proxy_info;
    {
        Spinner s("Fetching IP information (Direct & Proxy)...");
        local_info = APIClient::get_ip_info_direct();
        proxy_info = APIClient::get_ip_info_proxy(mixed_port);
    }
    
    std::cout << color::GREEN << "✔ IP information retrieved successfully:" << color::RESET << "\n\n";
    
    int lines_to_clear = 2; // header + blank
    
    if (local_info.success) {
        std::vector<std::string> parts = {local_info.country, local_info.region, local_info.city};
        std::string loc = "";
        for (auto& p : parts) if (!p.empty()) loc += p + " ";
        std::cout << "  " << color::BOLD << "本地 IP (Direct):" << color::RESET << "\n"
                  << "    IP Address: " << color::GREEN << local_info.ip << color::RESET << "\n"
                  << "    Location:   " << color::YELLOW << loc << color::RESET << "\n";
        lines_to_clear += 3;
        if (!local_info.isp.empty()) {
            std::cout << "    ISP:        " << color::CYAN << local_info.isp << color::RESET << "\n";
            lines_to_clear += 1;
        }
    } else {
        std::cout << "  " << color::BOLD << "本地 IP (Direct):" << color::RESET << " " << color::RED << "Failed to query" << color::RESET << "\n";
        lines_to_clear += 1;
    }
    
    std::cout << "\n";
    lines_to_clear += 1;
    
    if (proxy_info.success) {
        std::vector<std::string> parts = {proxy_info.country, proxy_info.region, proxy_info.city};
        std::string loc = "";
        for (auto& p : parts) if (!p.empty()) loc += p + " ";
        std::cout << "  " << color::BOLD << "代理 IP (Proxy):" << color::RESET << "\n"
                  << "    IP Address: " << color::GREEN << proxy_info.ip << color::RESET << "\n"
                  << "    Location:   " << color::YELLOW << loc << color::RESET << "\n";
        lines_to_clear += 3;
        if (!proxy_info.isp.empty()) {
            std::cout << "    ISP:        " << color::CYAN << proxy_info.isp << color::RESET << "\n";
            lines_to_clear += 1;
        }
    } else {
        std::cout << "  " << color::BOLD << "代理 IP (Proxy):" << color::RESET << " " << color::RED << "Failed to query (Is Mihomo running?)" << color::RESET << "\n";
        lines_to_clear += 1;
    }
    
    std::cout << "\n" << color::CYAN << "Press ESC to return to menu... (按 ESC 键返回主菜单)" << color::RESET << "\n";
    lines_to_clear += 2; // blank + prompt
    
    while (true) {
        KeyInput k = get_key();
        if (k.type == Key::ESC) {
            break;
        } else if (k.type == Key::CTRL_C) {
            std::cout << color::RESET << "\n";
            std::exit(0);
        }
    }
    
    std::cout << "\033[" << lines_to_clear << "A\033[J" << std::flush;
}

void interactive_select_node(APIClient& client) {
    auto selectors = client.get_selectors();
    if (selectors.empty()) {
        std::cout << color::RED << "✖ Error: Cannot fetch proxy groups. Is Mihomo running?" << color::RESET << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        std::cout << "\033[1A\033[J" << std::flush;
        return;
    }
    
    std::string group_name = "GLOBAL";
    if (selectors.size() > 1) {
        std::vector<std::string> group_items;
        for (auto& [name, info] : selectors) {
            group_items.push_back(name + " (current: " + info.now + ")");
        }
        auto idx = select_from_list("选择要修改的策略组 (Select Proxy Group):", group_items);
        if (!idx.has_value()) {
            return;
        }
        group_name = selectors[*idx].first;
    } else {
        group_name = selectors[0].first;
    }
    
    APIClient::SelectorInfo group_info;
    for (auto& [name, info] : selectors) {
        if (name == group_name) {
            group_info = info;
            break;
        }
    }
    
    int default_node_idx = 0;
    for (size_t i = 0; i < group_info.all.size(); ++i) {
        if (group_info.all[i] == group_info.now) {
            default_node_idx = i;
            break;
        }
    }
    
    auto selected_node = select_from_list_with_search("选择 [" + group_name + "] 策略组的节点 (可直接打字搜索):", group_info.all, default_node_idx);
    if (selected_node.has_value()) {
        bool success = false;
        {
            Spinner s("Switching [" + group_name + "] -> " + *selected_node + "...");
            success = client.switch_node(group_name, *selected_node);
        }
        if (success) {
            std::cout << color::GREEN << "✔ Switched successfully to " << *selected_node << color::RESET << "\n";
        } else {
            std::cout << color::RED << "✖ Switch failed." << color::RESET << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        std::cout << "\033[1A\033[J" << std::flush;
    }
}

void run_select_cli(APIClient& client, const std::string& query) {
    auto selectors = client.get_selectors();
    if (selectors.empty()) {
        std::cout << color::RED << "✖ Error: Cannot fetch proxy groups. Is Mihomo running?" << color::RESET << "\n";
        return;
    }
    
    std::string group_name = "GLOBAL";
    bool found_group = false;
    for (auto& [name, info] : selectors) {
        if (name == "三毛机场") {
            group_name = name;
            found_group = true;
            break;
        }
    }
    if (!found_group) {
        for (auto& [name, info] : selectors) {
            if (name == "GLOBAL") {
                group_name = name;
                found_group = true;
                break;
            }
        }
    }
    if (!found_group) {
        group_name = selectors[0].first;
    }
    
    APIClient::SelectorInfo group_info;
    for (auto& [name, info] : selectors) {
        if (name == group_name) {
            group_info = info;
            break;
        }
    }
    
    std::vector<std::string> matched;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (auto& n : group_info.all) {
        std::string lower_n = n;
        std::transform(lower_n.begin(), lower_n.end(), lower_n.begin(), ::tolower);
        if (lower_n.find(lower_query) != std::string::npos) {
            matched.push_back(n);
        }
    }
    
    if (matched.empty()) {
        std::cout << color::RED << "✖ No nodes matched query '" << query << "'." << color::RESET << "\n";
        return;
    }
    
    if (matched.size() == 1) {
        std::string target_node = matched[0];
        bool success = false;
        {
            Spinner s("Switching [" + group_name + "] -> " + target_node + "...");
            success = client.switch_node(group_name, target_node);
        }
        if (success) {
            std::cout << color::GREEN << "✔ Switched successfully to " << target_node << color::RESET << "\n";
        } else {
            std::cout << color::RED << "✖ Switch failed." << color::RESET << "\n";
        }
    } else {
        auto selected = select_from_list_with_search("Multiple matches for '" + query + "':", matched);
        if (selected.has_value()) {
            bool success = false;
            {
                Spinner s("Switching [" + group_name + "] -> " + *selected + "...");
                success = client.switch_node(group_name, *selected);
            }
            if (success) {
                std::cout << color::GREEN << "✔ Switched successfully to " << *selected << color::RESET << "\n";
            } else {
                std::cout << color::RED << "✖ Switch failed." << color::RESET << "\n";
            }
        }
    }
}

void show_status(APIClient& client) {
    auto [running, pid] = Service::get_status();
    std::cout << "\n" << color::BOLD << "Mihomo Service Status:" << color::RESET << "\n";
    if (running) {
        std::cout << "  Status: " << color::GREEN << "● RUNNING" << color::RESET << "\n"
                  << "  PID:    " << color::CYAN << pid << color::RESET << "\n";
        auto mode = client.get_mode();
        if (mode.has_value()) {
            std::cout << "  Mode:   " << color::GREEN << *mode << color::RESET << "\n";
        }
        auto selectors = client.get_selectors();
        if (!selectors.empty()) {
            std::cout << "  Active Proxy Groups:\n";
            for (auto& [name, info] : selectors) {
                std::cout << "    - " << name << ": " << color::YELLOW << info.now << color::RESET << "\n";
            }
        }
    } else {
        std::cout << "  Status: " << color::RED << "○ STOPPED" << color::RESET << "\n";
    }
}

void run_manage_subscriptions_menu() {
    while (true) {
        std::string active_url = "";
        std::vector<Subscription> list = SubscriptionManager::load(active_url);
        
        std::vector<std::string> sub_menu_items = {
            "Select Active Subscription (选择生效订阅)",
            "Add New Subscription       (增加订阅)",
            "Delete Subscription        (删除订阅)",
            "Update Subscription        (更新配置订阅)",
            "Return to Main Menu        (返回主菜单)"
        };
        
        std::cout << "\n============================================================\n"
                  << "               " << color::BOLD << color::BLUE << "Manage Subscriptions (订阅管理)" << color::RESET << "\n"
                  << "============================================================\n";
        
        if (list.empty()) {
            std::cout << "  No subscriptions configured. (当前无订阅记录)\n";
        } else {
            std::cout << "  Current Subscriptions:\n";
            for (auto& sub : list) {
                bool is_active = (sub.url == active_url && !active_url.empty());
                std::cout << "    " << (is_active ? (color::GREEN + "● " + color::RESET) : "  ") 
                          << color::BOLD << sub.name << color::RESET << "\n"
                          << "      URL: " << color::CYAN << sub.url << color::RESET << "\n";
            }
        }
        std::cout << "------------------------------------------------------------\n";
        
        auto choice = select_from_list("请选择操作:", sub_menu_items);
        
        int lines_to_clear = 3 + 1; // Header + separator
        if (list.empty()) {
            lines_to_clear += 1;
        } else {
            lines_to_clear += 1 + list.size() * 2;
        }
        
        std::cout << "\033[" << lines_to_clear << "A\033[J" << std::flush;
        
        if (!choice.has_value() || *choice == 4) {
            break;
        }
        
        if (*choice == 0) {
            if (list.empty()) {
                std::cout << color::RED << "✖ No subscriptions to select." << color::RESET << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                std::cout << "\033[1A\033[J" << std::flush;
                continue;
            }
            std::vector<std::string> items;
            for (auto& sub : list) {
                bool is_active = (sub.url == active_url && !active_url.empty());
                items.push_back(sub.name + (is_active ? " (Active)" : ""));
            }
            auto sel = select_from_list("选择生效的订阅:", items);
            if (sel.has_value()) {
                active_url = list[*sel].url;
                SubscriptionManager::save(list, active_url);
                std::cout << color::GREEN << "✔ Active subscription set to: " << list[*sel].name << color::RESET << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                std::cout << "\033[1A\033[J" << std::flush;
            }
        }
        else if (*choice == 1) {
            std::cout << "\n";
            std::string name = read_input("输入订阅名称 (Name): ");
            while (!name.empty() && (name.back() == '\r' || name.back() == '\n' || name.back() == ' ')) name.pop_back();
            if (name.empty()) {
                std::cout << color::RED << "✖ Name cannot be empty." << color::RESET << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                std::cout << "\033[3A\033[J" << std::flush;
                continue;
            }
            
            std::string url = read_input("输入订阅 URL: ");
            while (!url.empty() && (url.back() == '\r' || url.back() == '\n' || url.back() == ' ')) url.pop_back();
            if (url.empty()) {
                std::cout << color::RED << "✖ URL cannot be empty." << color::RESET << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                std::cout << "\033[4A\033[J" << std::flush;
                continue;
            }
            
            Subscription sub;
            sub.name = name;
            sub.url = url;
            list.push_back(sub);
            
            if (list.size() == 1) {
                active_url = url;
            }
            
            SubscriptionManager::save(list, active_url);
            std::cout << color::GREEN << "✔ Subscription '" << name << "' added successfully!" << color::RESET << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
            std::cout << "\033[4A\033[J" << std::flush;
        }
        else if (*choice == 2) {
            if (list.empty()) {
                std::cout << color::RED << "✖ No subscriptions to delete." << color::RESET << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                std::cout << "\033[1A\033[J" << std::flush;
                continue;
            }
            std::vector<std::string> items;
            for (auto& sub : list) {
                items.push_back(sub.name);
            }
            auto sel = select_from_list("选择要删除的订阅:", items);
            if (sel.has_value()) {
                std::string del_name = list[*sel].name;
                std::string del_url = list[*sel].url;
                list.erase(list.begin() + *sel);
                
                if (active_url == del_url) {
                    if (!list.empty()) {
                        active_url = list[0].url;
                    } else {
                        active_url = "";
                    }
                }
                
                SubscriptionManager::save(list, active_url);
                std::cout << color::GREEN << "✔ Subscription '" << del_name << "' deleted." << color::RESET << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                std::cout << "\033[1A\033[J" << std::flush;
            }
        }
        else if (*choice == 3) {
            bool success = false;
            std::string msg;
            {
                Spinner s("Updating subscription configurations...");
                auto res = Service::update_subscription();
                success = res.first;
                msg = res.second;
            }
            if (success) {
                std::cout << color::GREEN << "✔ " << msg << color::RESET << "\n";
            } else {
                std::cout << color::RED << "✖ " << msg << color::RESET << "\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            std::cout << "\033[1A\033[J" << std::flush;
        }
    }
}

void run_main_menu(Config& conf) {
    APIClient client(conf.ctrl_port, conf.secret);
    std::vector<std::string> menu_items = {
        "Start Service              (启动服务)",
        "Stop Service               (停止服务)",
        "Switch Mode                (切换模式: Rule/Global/Direct)",
        "Select Node                (切换代理节点)",
        "Manage Subscriptions       (管理订阅)",
        "Query IP & Location       (查询公网IP及归属地)",
        "Show Live Logs             (查看实时日志)",
        "Exit                       (退出管理)"
    };
    
    while (true) {
        auto [running, pid] = Service::get_status();
        
        std::cout << "============================================================\n"
                  << "          " << color::BOLD << color::BLUE << "Mihomo (Clash Meta) CLI Manager" << color::RESET << "\n"
                  << "============================================================\n";
                  
        if (running) {
            std::string mode = "Unknown";
            auto mode_opt = client.get_mode();
            if (mode_opt) mode = *mode_opt;
            
            auto selectors = client.get_selectors();
            std::string node_str = "None";
            if (!selectors.empty()) {
                std::string target_g = "";
                for (auto& [name, info] : selectors) {
                    if (name == "三毛机场") { target_g = name; break; }
                }
                if (target_g.empty()) {
                    for (auto& [name, info] : selectors) {
                        if (name == "GLOBAL") { target_g = name; break; }
                    }
                }
                if (target_g.empty()) {
                    target_g = selectors[0].first;
                }
                
                for (auto& [name, info] : selectors) {
                    if (name == target_g) {
                        node_str = name + " -> " + info.now;
                        break;
                    }
                }
            }
            std::cout << " Status: " << color::GREEN << "● RUNNING (PID: " << pid << ")" << color::RESET 
                      << " | Mode: " << color::CYAN << mode << color::RESET << "\n"
                      << " Node:   " << color::YELLOW << node_str << color::RESET << "\n";
        } else {
            std::cout << " Status: " << color::RED << "○ STOPPED" << color::RESET << "\n";
        }
        std::cout << "------------------------------------------------------------\n";
        
        auto choice = select_from_list("请使用 ↑↓ 键移动，Enter 确认选择功能:", menu_items);
        
        int lines_to_clear = running ? 6 : 5;
        std::cout << "\033[" << lines_to_clear << "A\033[J" << std::flush;
        
        if (!choice.has_value() || *choice == 7) {
            std::cout << "Goodbye!\n";
            break;
        }
        
        if (*choice == 0) {
            bool success = false;
            std::string msg;
            {
                Spinner s("Starting Mihomo proxy service...");
                auto res = Service::start_service();
                success = res.first;
                msg = res.second;
            }
            if (success) {
                std::cout << color::GREEN << "✔ " << msg << color::RESET << "\n";
            } else {
                std::cout << color::RED << "✖ " << msg << color::RESET << "\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            std::cout << "\033[1A\033[J" << std::flush;
        }
        else if (*choice == 1) {
            bool success = false;
            std::string msg;
            {
                Spinner s("Stopping Mihomo proxy service...");
                auto res = Service::stop_service();
                success = res.first;
                msg = res.second;
            }
            if (success) {
                std::cout << color::GREEN << "✔ " << msg << color::RESET << "\n";
            } else {
                std::cout << color::RED << "✖ " << msg << color::RESET << "\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            std::cout << "\033[1A\033[J" << std::flush;
        }
        else if (*choice == 2) {
            if (!running) {
                std::cout << color::RED << "✖ Service is not running. Please start it first." << color::RESET << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                std::cout << "\033[1A\033[J" << std::flush;
                continue;
            }
            std::vector<std::string> modes = {"Rule (规则模式)", "Global (全局模式)", "Direct (直连模式)"};
            auto mode_choice = select_from_list("选择运行模式 (Switch Mode):", modes);
            if (mode_choice.has_value()) {
                if (*mode_choice == 0) {
                    client.set_mode("rule");
                } else if (*mode_choice == 1) {
                    client.set_mode("global");
                } else if (*mode_choice == 2) {
                    client.set_mode("direct");
                }
            }
        }
        else if (*choice == 3) {
            if (!running) {
                std::cout << color::RED << "✖ Service is not running. Please start it first." << color::RESET << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                std::cout << "\033[1A\033[J" << std::flush;
                continue;
            }
            interactive_select_node(client);
        }
        else if (*choice == 4) {
            run_manage_subscriptions_menu();
        }
        else if (*choice == 5) {
            check_ip_command_with_spinner(conf.mixed_port);
        }
        else if (*choice == 6) {
            std::cout << "\n" << color::CYAN << "Showing logs (Press Ctrl+C to return to menu)..." << color::RESET << "\n\n";
            Service::show_live_logs();
            std::cout << "\nReturned to menu.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
            std::cout << "\033[4A\033[J" << std::flush;
        }
    }
}

int main(int argc, char* argv[]) {
    Config conf;
    conf.load();

    APIClient client(conf.ctrl_port, conf.secret);

    if (argc < 2) {
        try {
            run_main_menu(conf);
        } catch (...) {
            std::cout << "\033[?25h\r\nGoodbye!\n";
            return 0;
        }
        return 0;
    }

    std::string cmd = argv[1];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        print_help();
    }
    else if (cmd == "start") {
        bool success = false;
        std::string msg;
        {
            Spinner s("Starting Mihomo proxy service...");
            auto res = Service::start_service();
            success = res.first;
            msg = res.second;
        }
        if (success) {
            std::cout << color::GREEN << "✔ " << msg << color::RESET << "\n";
        } else {
            std::cout << color::RED << "✖ " << msg << color::RESET << "\n";
        }
    }
    else if (cmd == "stop") {
        bool success = false;
        std::string msg;
        {
            Spinner s("Stopping Mihomo proxy service...");
            auto res = Service::stop_service();
            success = res.first;
            msg = res.second;
        }
        if (success) {
            std::cout << color::GREEN << "✔ " << msg << color::RESET << "\n";
        } else {
            std::cout << color::RED << "✖ " << msg << color::RESET << "\n";
        }
    }
    else if (cmd == "restart") {
        std::string start_msg;
        bool start_success = false;
        {
            Spinner s("Restarting Mihomo proxy service...");
            Service::stop_service();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto res = Service::start_service();
            start_success = res.first;
            start_msg = res.second;
        }
        if (start_success) {
            std::cout << color::GREEN << "✔ Service restarted successfully." << color::RESET << "\n";
        } else {
            std::cout << color::RED << "✖ Failed to restart: " << start_msg << color::RESET << "\n";
        }
    }
    else if (cmd == "status") {
        show_status(client);
    }
    else if (cmd == "mode") {
        auto [running, _] = Service::get_status();
        if (!running) {
            std::cout << color::RED << "✖ Mihomo is not running." << color::RESET << "\n";
            return 0;
        }
        if (argc < 3) {
            auto mode = client.get_mode();
            if (mode) {
                std::cout << "Current Mode: " << color::GREEN << *mode << color::RESET << "\n";
            } else {
                std::cout << color::RED << "✖ Failed to query mode." << color::RESET << "\n";
            }
        } else {
            std::string mode_arg = argv[2];
            if (client.set_mode(mode_arg)) {
                std::cout << color::GREEN << "✔ Mode switched to " << mode_arg << color::RESET << "\n";
            } else {
                std::cout << color::RED << "✖ Failed to switch mode. Use rule, global, or direct." << color::RESET << "\n";
            }
        }
    }
    else if (cmd == "select") {
        auto [running, _] = Service::get_status();
        if (!running) {
            std::cout << color::RED << "✖ Mihomo is not running." << color::RESET << "\n";
            return 0;
        }
        if (argc < 3) {
            interactive_select_node(client);
        } else {
            std::string query = "";
            for (int i = 2; i < argc; ++i) {
                if (i > 2) query += " ";
                query += argv[i];
            }
            run_select_cli(client, query);
        }
    }
    else if (cmd == "update") {
        bool success = false;
        std::string msg;
        {
            Spinner s("Updating subscription configurations...");
            auto res = Service::update_subscription();
            success = res.first;
            msg = res.second;
        }
        if (success) {
            std::cout << color::GREEN << "✔ " << msg << color::RESET << "\n";
        } else {
            std::cout << color::RED << "✖ " << msg << color::RESET << "\n";
        }
    }
    else if (cmd == "ip" || cmd == "myip") {
        run_ip_cli(conf.mixed_port);
    }
    else if (cmd == "log") {
        std::cout << color::CYAN << "Showing logs (Press Ctrl+C to exit)..." << color::RESET << "\n\n";
        Service::show_live_logs();
    }
    else {
        std::cout << color::RED << "✖ Unknown command: " << cmd << color::RESET << "\n";
        print_help();
    }

    return 0;
}
