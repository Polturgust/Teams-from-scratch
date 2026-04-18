/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Constructor.cpp
*/

#include "Client.hpp"

Client::Client(const std::string &ip, int port)
    : _fd(-1), _running(false)
{
    _connect(ip, port);
}

Client::~Client()
{
    if (_fd >= 0)
        close(_fd);
}

void Client::_connect(const std::string &ip, int port)
{
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0) {
        std::cerr << "socket: " << strerror(errno) << std::endl;
        exit(84);
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address: " << ip << std::endl;
        exit(84);
    }

    if (connect(_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::cerr << "connect: " << strerror(errno) << std::endl;
        exit(84);
    }
}
