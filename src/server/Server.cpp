/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Server.cpp
*/

#include "Server.hpp"
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <stdexcept>


Server::Server(int port)
    : _port(port), _running(false), _business(data)
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

    int client_fd = accept(_server_fd,
                           (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
        return;

    ClientState state = {};
    state.fd = client_fd;
    clients[client_fd] = state;
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

void Server::shutdown()
{
    _running = false;
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

    auto it = data.sessions.find(fd);
    if (it != data.sessions.end()) {
        mtp::Result r = _business.handle_logout(fd);
        for (auto &push : r.pushes) {
            for (int target_fd : push.fds) {
                if (target_fd == fd)
                    continue;
                _queue_bytes(target_fd, push.packet.bytes);
            }
        }
    }

    close(fd);
    clients.erase(fd);
}

void Server::_queue_bytes(int fd, const std::vector<uint8_t> &bytes)
{
    if (clients.find(fd) == clients.end())
        return;
    auto &buf = clients[fd].send_buffer;
    buf.insert(buf.end(), bytes.begin(), bytes.end());
}

void Server::_send_result(int fd, const mtp::Result &result)
{
    if (!result.response.bytes.empty())
        _queue_bytes(fd, result.response.bytes);
    for (const auto &push : result.pushes) {
        for (int target_fd : push.fds) {
            if (target_fd == fd)
                continue;
            _queue_bytes(target_fd, push.packet.bytes);
        }
    }
}


static inline std::string_view extract_sv(const std::vector<uint8_t> &p, size_t offset, size_t size)
{
    if (offset + size > p.size())
        return {};
    return std::string_view((const char *)p.data() + offset, size);
}

void Server::_dispatch(int fd, uint16_t cmd, const std::vector<uint8_t> &payload)
{
    const bool logged = (data.sessions.find(fd) != data.sessions.end());
    if (!logged && cmd != CMD_LOGIN) {
        uint16_t c = htons(ERR_UNAUTHORIZED);
        uint32_t s = htonl(0);
        std::vector<uint8_t> err;
        err.insert(err.end(), (uint8_t*)&c, (uint8_t*)&c + 2);
        err.insert(err.end(), (uint8_t*)&s, (uint8_t*)&s + 4);
        _queue_bytes(fd, err);
        return;
    }

    mtp::Result r;

    switch (cmd) {
        case CMD_LOGIN:
            r = _business.handle_login(fd, extract_sv(payload, 0, payload.size()));
            break;
        case CMD_LOGOUT:
            r = _business.handle_logout(fd);
            break;
        case CMD_USERS:
            r = _business.handle_users(fd);
            break;
        case CMD_USER:
            r = _business.handle_user(fd, extract_sv(payload, 0, 36));
            break;
        case CMD_SEND:
            r = _business.handle_send(fd,
                extract_sv(payload, 0, 36),
                extract_sv(payload, 36, MAX_BODY_LENGTH));
            break;
        case CMD_MESSAGES:
            r = _business.handle_messages(fd, extract_sv(payload, 0, 36));
            break;
        case CMD_SUBSCRIBE:
            r = _business.handle_subscribe(fd, extract_sv(payload, 0, 36));
            break;
        case CMD_SUBSCRIBED:
            if (!payload.empty() && payload[0] == 0x01)
                r = _business.handle_subscribed_users(fd, extract_sv(payload, 1, 36));
            else
                r = _business.handle_subscribed_teams(fd);
            break;
        case CMD_UNSUBSCRIBE:
            r = _business.handle_unsubscribe(fd, extract_sv(payload, 0, 36));
            break;
        case CMD_USE:
            r = _business.handle_use(fd, extract_sv(payload, 0, payload.size()));
            break;
        case CMD_CREATE:
            r = _business.handle_create(fd, extract_sv(payload, 0, payload.size()));
            break;
        case CMD_LIST:
            r = _business.handle_list(fd);
            break;
        case CMD_INFO:
            r = _business.handle_info(fd);
            break;
            return;
        default: {
            uint16_t c = htons(ERR_INVALID_COMMAND);
            uint32_t s = htonl(0);
            std::vector<uint8_t> err;
            err.insert(err.end(), (uint8_t*)&c, (uint8_t*)&c + 2);
            err.insert(err.end(), (uint8_t*)&s, (uint8_t*)&s + 4);
            _queue_bytes(fd, err);
            return;
        }
    }
    _send_result(fd, r);
}