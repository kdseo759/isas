#include "udp_server.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>

#define BROADCAST_PORT 9000

UdpServer::UdpServer(const std::vector<std::string>& iface_names) {
    for (const auto& iface_name : iface_names) {
        setupInterface(iface_name);
    }
}

UdpServer::~UdpServer() {
    for (auto& iface : interfaces) {
        if (iface.sockfd >= 0) {
            close(iface.sockfd);
        }
    }
}

void UdpServer::setupInterface(const std::string& iface_name) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Socket creation failed for " + iface_name);
    }

    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        close(sockfd);
        throw std::runtime_error("Failed to set broadcast option for " + iface_name);
    }

    std::string broadcast_ip = getBroadcastAddress(iface_name);

    if(broadcast_ip.empty() || broadcast_ip == "") return;
    interfaces.push_back({sockfd, broadcast_ip});
}

std::string UdpServer::getBroadcastAddress(const std::string& iface_name) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        throw std::runtime_error("Failed to get network interfaces");
    }

    std::string broadcast_ip;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || iface_name != ifa->ifa_name) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            auto* addr = (struct sockaddr_in*)ifa->ifa_addr;
            auto* netmask = (struct sockaddr_in*)ifa->ifa_netmask;
            uint32_t ip = addr->sin_addr.s_addr;
            uint32_t mask = netmask->sin_addr.s_addr;
            uint32_t broadcast = ip | ~mask;
            char broadcast_ip_cstr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &broadcast, broadcast_ip_cstr, INET_ADDRSTRLEN);
            broadcast_ip = broadcast_ip_cstr;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return broadcast_ip;
}

void UdpServer::sendBroadcast(const std::array<uint8_t, 3>& data, std::ofstream& log_file) {
    for (const auto& iface : interfaces) {
        struct sockaddr_in broadcast_addr {};
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_port = htons(BROADCAST_PORT);
        inet_pton(AF_INET, iface.broadcast_ip_addr.c_str(), &broadcast_addr.sin_addr);

        if (sendto(iface.sockfd, data.data(), data.size(), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
            std::cerr << "Error: sendto failed on interface " << iface.broadcast_ip_addr << "\n";
        } else {
            logMessage(log_file, "BroadcastSend " + iface.broadcast_ip_addr + ": " +
                std::to_string(data[0]) + "," + std::to_string(data[1]) + "," + std::to_string(data[2]));
            std::cout << "Broadcast send " << iface.broadcast_ip_addr << ": "
                      << (int)data[0] << " " << (int)data[1] << " " << (int)data[2] << "\n";
        }
    }
}
