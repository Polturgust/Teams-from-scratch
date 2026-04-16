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

void Client::_handle_server_read()
{
    uint8_t tmp[4096];
    ssize_t n = read(_fd, tmp, sizeof(tmp));
    if (n <= 0) {
        std::cerr << "Server disconnected." << std::endl;
        _running = false;
        return;
    }
    _recv_buf.insert(_recv_buf.end(), tmp, tmp + n);
    _process_recv_buffer();
}

void Client::_handle_server_write()
{
    if (_send_buf.empty()) return;
    ssize_t n = write(_fd, _send_buf.data(), _send_buf.size());
    if (n <= 0) return;
    _send_buf.erase(_send_buf.begin(), _send_buf.begin() + n);
}

void Client::_process_recv_buffer()
{
    const size_t HEADER = 6;
    while (_recv_buf.size() >= HEADER) {
        uint16_t code;
        uint32_t psize;
        std::memcpy(&code,  _recv_buf.data(),     2);
        std::memcpy(&psize, _recv_buf.data() + 2, 4);
        code  = ntohs(code);
        psize = ntohl(psize);

        if (_recv_buf.size() < HEADER + psize) break; // wait for full payload

        std::vector<uint8_t> payload(
            _recv_buf.begin() + HEADER,
            _recv_buf.begin() + HEADER + psize);
        _recv_buf.erase(_recv_buf.begin(), _recv_buf.begin() + HEADER + psize);

        _handle_response(code, payload);
    }
}

void Client::_send_packet(uint16_t cmd, const void *payload, uint32_t size)
{
    uint16_t cmd_net  = htons(cmd);
    uint32_t size_net = htonl(size);

    uint8_t header[6];
    std::memcpy(header,     &cmd_net,  2);
    std::memcpy(header + 2, &size_net, 4);

    _send_buf.insert(_send_buf.end(), header, header + 6);
    if (payload && size > 0)
        _send_buf.insert(_send_buf.end(),
            static_cast<const uint8_t *>(payload),
            static_cast<const uint8_t *>(payload) + size);

    _handle_server_write();
}

void Client::_handle_stdin()
{
    std::string line;
    if (!std::getline(std::cin, line)) {
        _running = false;
        return;
    }
    if (line.empty()) return;

    ParsedCommand cmd = _parse_line(line);
    if (!cmd.valid) {
        std::cerr << "Parse error: " << cmd.error << std::endl;
        return;
    }
    _dispatch(cmd);
}

Client::ParsedCommand Client::_parse_line(const std::string &line)
{
    ParsedCommand result;

    if (line.empty() || line[0] != '/') {
        result.valid = false;
        result.error = "Commands must start with '/'";
        return result;
    }

    size_t i = 1;
    while (i < line.size() && line[i] != ' ') ++i;
    result.name = line.substr(1, i - 1);

    while (i < line.size()) {
        while (i < line.size() && line[i] == ' ') ++i;
        if (i >= line.size()) break;

        if (line[i] != '"') {
            result.valid = false;
            result.error = "Arguments must be enclosed in double quotes";
            return result;
        }
        ++i;
        std::string arg;
        bool closed = false;
        while (i < line.size()) {
            if (line[i] == '"') {
                closed = true;
                ++i;
                break;
            }
            arg.push_back(line[i++]);
        }
        if (!closed) {
            result.valid = false;
            result.error = "Unclosed double quote in argument";
            return result;
        }
        result.args.push_back(arg);
    }
    return result;
}

void Client::_dispatch(const ParsedCommand &cmd)
{
    if (cmd.name == "help")        { _cmd_help(); return; }
    if (cmd.name == "login")       { _cmd_login(cmd); return; }
    if (cmd.name == "logout")      { _cmd_logout(cmd); return; }
    if (cmd.name == "users")       { _cmd_users(cmd); return; }
    if (cmd.name == "user")        { _cmd_user(cmd); return; }
    if (cmd.name == "send")        { _cmd_send(cmd); return; }
    if (cmd.name == "messages")    { _cmd_messages(cmd); return; }
    if (cmd.name == "subscribe")   { _cmd_subscribe(cmd); return; }
    if (cmd.name == "subscribed")  { _cmd_subscribed(cmd); return; }
    if (cmd.name == "unsubscribe") { _cmd_unsubscribe(cmd); return; }
    if (cmd.name == "use")         { _cmd_use(cmd); return; }
    if (cmd.name == "create")      { _cmd_create(cmd); return; }
    if (cmd.name == "list")        { _cmd_list(cmd); return; }
    if (cmd.name == "info")        { _cmd_info(cmd); return; }

    std::cerr << "Unknown command: /" << cmd.name << ". Type /help for help." << std::endl;
}

void Client::_cmd_help()
{
    std::cout <<
        "Available commands:\n"
        "  /help                                        Show this help\n"
        "  /login \"user_name\"                           Log in to the server\n"
        "  /logout                                      Disconnect from the server\n"
        "  /users                                       List all users\n"
        "  /user \"user_uuid\"                            Get details about a user\n"
        "  /send \"user_uuid\" \"message_body\"             Send a private message\n"
        "  /messages \"user_uuid\"                        List messages with a user\n"
        "  /subscribe \"team_uuid\"                       Subscribe to a team\n"
        "  /subscribed [\"team_uuid\"]                    List subscribed teams or team members\n"
        "  /unsubscribe \"team_uuid\"                     Unsubscribe from a team\n"
        "  /use [\"team_uuid\" [\"channel_uuid\" [\"thread_uuid\"]]]  Set context\n"
        "  /create [args...]                            Create resource (context-dependent)\n"
        "    (no context)          \"team_name\" \"team_description\"\n"
        "    (team)                \"channel_name\" \"channel_description\"\n"
        "    (team+channel)        \"thread_title\" \"thread_body\"\n"
        "    (team+channel+thread) \"comment_body\"\n"
        "  /list                                        List resources (context-dependent)\n"
        "  /info                                        Show info (context-dependent)\n";
}
