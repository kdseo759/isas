#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <array>
#include <string>
#include <mutex>

class UdpServer {
public:
    UdpServer(const std::string& iface_name);
    ~UdpServer();
    void sendBroadcast(const std::array<uint8_t, 3>& data);

private:
    int sockfd;
    std::string iface_name;
    std::string broadcast_ip_addr;
    std::mutex log_mutex;

    void getBroadcastAddress();
};

#endif