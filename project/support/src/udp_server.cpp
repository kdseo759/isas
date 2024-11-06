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

UdpServer::UdpServer(const std::string &iface_name) : iface_name(iface_name)
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        throw std::runtime_error("Socket creation failed");
    }

    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
    {
        close(sockfd);
        throw std::runtime_error("Failed to set broadcast option");
    }

    getBroadcastAddress();
}

UdpServer::~UdpServer()
{
    if (sockfd >= 0)
    {
        close(sockfd);
    }
}

void UdpServer::sendBroadcast(const std::array<uint8_t, 3> &data)
{
    struct sockaddr_in broadcast_addr;
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(BROADCAST_PORT);
    inet_pton(AF_INET, broadcast_ip_addr.c_str(), &broadcast_addr.sin_addr);

    int res = sendto(sockfd, data.data(), data.size(), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
    if (res < 0)
    {
        std::cerr << "Error: sendto failed\n";
    }
    else
    {
        std::cout << "SendToBroadCast Data [" << broadcast_ip_addr << "] ";
        for (auto byte : data)
        {
            std::cout << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
    }
}

void UdpServer::getBroadcastAddress()
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
    {
        throw std::runtime_error("Failed to get network interfaces");
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL || iface_name != ifa->ifa_name)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            auto *addr = (struct sockaddr_in *)ifa->ifa_addr;
            auto *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
            uint32_t ip = addr->sin_addr.s_addr;
            uint32_t mask = netmask->sin_addr.s_addr;
            uint32_t broadcast = ip | ~mask;
            char broadcast_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &broadcast, broadcast_ip, INET_ADDRSTRLEN);
            broadcast_ip_addr = broadcast_ip;
            std::cout << "GetIP " << broadcast_ip_addr << "\n";
            break;
        }
    }

    freeifaddrs(ifaddr);
}