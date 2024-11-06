#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <array>
#include <string>
#include <mutex>
#include <vector>
#include <fstream>
#include <ctime>
#include <iomanip>

class UdpServer {
public:
    UdpServer(const std::vector<std::string>& iface_names);
    ~UdpServer();
    void sendBroadcast(const std::array<uint8_t, 3>& data, std::ofstream& log_file);

private:
    struct InterfaceInfo {
        int sockfd;
        std::string broadcast_ip_addr;
    };

    std::vector<InterfaceInfo> interfaces;
    std::string broadcast_ip_addr;
    std::mutex log_mutex;

    void setupInterface(const std::string& iface_name);
    std::string getBroadcastAddress(const std::string& iface_name);
};

inline void logMessage(std::ofstream& log_file, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm now_tm = *std::localtime(&now_time_t);
    log_file << "[" << std::put_time(&now_tm, "%Y%m%d %H:%M:%S")
             << '.' << std::setw(3) << std::setfill('0') << now_ms.count() << "]"
             << message << std::endl;
}

#endif