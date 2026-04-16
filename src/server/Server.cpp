/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Server.cpp
*/

#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <algorithm>

Server::Server(int port) : _port(port), _running(false)
{
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
        throw std::runtime_error("socket() failed");
    int opt = 1;
    setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    if (bind(_server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(_server_fd);
        throw std::runtime_error("bind() failed");
    }

    if (listen(_server_fd, 10) < 0) {
        close(_server_fd);
        throw std::runtime_error("listen() failed");
    }
}

Server::~Server()
{
    for (auto &[fd, client] : clients)
        close(fd);
    close(_server_fd);
}

std::vector<struct pollfd> Server::_build_pollfds()
{
    std::vector<struct pollfd> fds;
    struct pollfd server_pfd = {};
    server_pfd.fd = _server_fd;
    server_pfd.events = POLLIN;
    fds.push_back(server_pfd);

    for (auto &[fd, client] : clients) {
        struct pollfd pfd = {};
        pfd.fd = fd;
        pfd.events = POLLIN;
        if (!client.send_buffer.empty())
            pfd.events |= POLLOUT;
        fds.push_back(pfd);
    }
    return fds;
}

void Server::_handle_new_connection()
{
    struct sockaddr_in client_addr = {};
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(_server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
        return;

    ClientState state = {};
    state.fd = client_fd;
    clients[client_fd] = state;
}

void Server::_process_recv_buffer(int fd)
{
    auto &buf = clients[fd].recv_buffer;

    while (buf.size() >= HEADER_SIZE) {

        uint16_t cmd = ntohs(*(uint16_t *)&buf[0]);
        uint32_t payload_size = ntohl(*(uint32_t *)&buf[2]);

        if (payload_size > 65535) {
            _handle_client_disconnect(fd);
            return;
        }
        if (buf.size() < HEADER_SIZE + payload_size)
            break;
        std::vector<uint8_t> payload(
            buf.begin() + HEADER_SIZE,
            buf.begin() + HEADER_SIZE + payload_size
        );
        buf.erase(buf.begin(), buf.begin() + HEADER_SIZE + payload_size);
        _dispatch(fd, cmd, payload);
        if (clients.find(fd) == clients.end())
            return;
    }
}

void Server::_handle_client_read(int fd)
{
    char tmp[4096];
    ssize_t n = read(fd, tmp, sizeof(tmp));

    if (n <= 0) {
        _handle_client_disconnect(fd);
        return;
    }
    auto &buf = clients[fd].recv_buffer;
    buf.insert(buf.end(), tmp, tmp + n);
    _process_recv_buffer(fd);
}

void Server::_handle_client_write(int fd)
{
    auto &buf = clients[fd].send_buffer;

    if (buf.empty())
        return;
    ssize_t n = write(fd, buf.data(), buf.size());
    if (n < 0) {
        _handle_client_disconnect(fd);
        return;
    }
    buf.erase(buf.begin(), buf.begin() + n);
}

void Server::_handle_client_disconnect(int fd)
{
    auto it = clients.find(fd);
    if (it != clients.end() && !it->second.user_uuid.empty()) {
        for (auto &user : data.users) {
            if (it->second.user_uuid == user.uuid) {
                user.is_logged = false;
                user.fd = -1;
                break;
            }
        }
    }
    close(fd);
    clients.erase(fd);
}

void Server::run()
{
    _running = true;

    while (_running) {
        std::vector<struct pollfd> pollfds = _build_pollfds();

        int ret = poll(pollfds.data(), pollfds.size(), -1);
        if (ret < 0) {
            if (errno == EINTR)
                break;
            throw std::runtime_error("poll() failed");
        }
        if (pollfds[0].revents & POLLIN)
            _handle_new_connection();
        std::vector<int> client_fds;
        for (auto &[fd, _] : clients)
            client_fds.push_back(fd);
        for (size_t i = 1; i < pollfds.size(); i++) {
            int fd = pollfds[i].fd;
        if (clients.find(fd) == clients.end())
                continue;
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                _handle_client_disconnect(fd);
                continue;
            }
            if (pollfds[i].revents & POLLIN)
                _handle_client_read(fd);
            if (clients.find(fd) == clients.end())
                continue;
            if (pollfds[i].revents & POLLOUT)
                _handle_client_write(fd);
        }
    }
}

void Server::queue_response(int fd, uint16_t cmd, const void *payload_data, uint32_t payload_size)
{
    if (clients.find(fd) == clients.end())
        return;

    auto &buf = clients[fd].send_buffer;
    uint16_t net_cmd = htons(cmd);
    uint32_t net_size = htonl(payload_size);
    buf.insert(buf.end(), (uint8_t *)&net_cmd, (uint8_t *)&net_cmd + sizeof(net_cmd));
    buf.insert(buf.end(), (uint8_t *)&net_size, (uint8_t *)&net_size + sizeof(net_size));

    if (payload_data && payload_size > 0) {
        const uint8_t *p = (const uint8_t *)payload_data;
        buf.insert(buf.end(), p, p + payload_size);
    }
}