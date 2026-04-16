/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Client.cpp
*/

#include "Client.hpp"
#include "../../libs/myteams/logging_client.h"

static void pack_fixed(std::vector<uint8_t> &buf, const std::string &s, size_t field_len)
{
    size_t copy_len = std::min(s.size(), field_len);
    for (size_t i = 0; i < copy_len; ++i)
        buf.push_back(static_cast<uint8_t>(s[i]));
    for (size_t i = copy_len; i < field_len; ++i)
        buf.push_back(0x00);
}

static std::string read_fixed(const std::vector<uint8_t> &p, size_t &off, size_t len)
{
    if (off + len > p.size()) return "";
    std::string s(reinterpret_cast<const char *>(p.data() + off), len);
    size_t end = s.find('\0');
    if (end != std::string::npos) s.resize(end);
    off += len;
    return s;
}

static uint32_t read_u32(const std::vector<uint8_t> &p, size_t &off)
{
    if (off + 4 > p.size()) return 0;
    uint32_t v;
    std::memcpy(&v, p.data() + off, 4);
    off += 4;
    return ntohl(v);
}

static uint8_t read_u8(const std::vector<uint8_t> &p, size_t &off)
{
    if (off >= p.size()) return 0;
    return p[off++];
}

static int64_t read_i64(const std::vector<uint8_t> &p, size_t &off)
{
    if (off + 8 > p.size()) return 0;
    int64_t v;
    std::memcpy(&v, p.data() + off, 8);
    off += 8;
    uint64_t u;
    std::memcpy(&u, &v, 8);
    u = ((u & 0x00000000000000FFULL) << 56)
      | ((u & 0x000000000000FF00ULL) << 40)
      | ((u & 0x0000000000FF0000ULL) << 24)
      | ((u & 0x00000000FF000000ULL) <<  8)
      | ((u & 0x000000FF00000000ULL) >>  8)
      | ((u & 0x0000FF0000000000ULL) >> 24)
      | ((u & 0x00FF000000000000ULL) >> 40)
      | ((u & 0xFF00000000000000ULL) >> 56);
    std::memcpy(&v, &u, 8);
    return v;
}

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

void Client::run()
{
    _running = true;
    while (_running) {
        struct pollfd fds[2];
        fds[0].fd     = STDIN_FILENO;
        fds[0].events = POLLIN;
        fds[1].fd     = _fd;
        fds[1].events = POLLIN | (_send_buf.empty() ? 0 : POLLOUT);

        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            std::cerr << "poll: " << strerror(errno) << std::endl;
            break;
        }

        if (fds[0].revents & POLLIN)
            _handle_stdin();

        if (fds[1].revents & POLLOUT)
            _handle_server_write();

        if (fds[1].revents & POLLIN)
            _handle_server_read();

        if (fds[1].revents & (POLLHUP | POLLERR)) {
            std::cerr << "Server disconnected." << std::endl;
            _running = false;
        }
    }
}
