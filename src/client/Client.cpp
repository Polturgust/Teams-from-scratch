/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Client.cpp
*/

#include "Client.hpp"

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

void Client::_cmd_login(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /login \"user_name\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 32);
    _send_packet(CMD_LOGIN, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_logout(const ParsedCommand &cmd)
{
    (void)cmd;
    _send_packet(CMD_LOGOUT);
}

void Client::_cmd_users(const ParsedCommand &cmd)
{
    (void)cmd;
    _send_packet(CMD_USERS);
}

void Client::_cmd_user(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /user \"user_uuid\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    _send_packet(CMD_USER, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_send(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 2) {
        std::cerr << "Usage: /send \"user_uuid\" \"message_body\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    pack_fixed(payload, cmd.args[1], 512);
    _send_packet(CMD_SEND, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_messages(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /messages \"user_uuid\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    _send_packet(CMD_MESSAGES, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_subscribe(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /subscribe \"team_uuid\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    _send_packet(CMD_SUBSCRIBE, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_subscribed(const ParsedCommand &cmd)
{
    std::vector<uint8_t> payload;
    if (cmd.args.empty()) {
        payload.push_back(0x00);
    } else if (cmd.args.size() == 1) {
        payload.push_back(0x01);
        pack_fixed(payload, cmd.args[0], 36);
    } else {
        std::cerr << "Usage: /subscribed [\"team_uuid\"]" << std::endl;
        return;
    }
    _send_packet(CMD_SUBSCRIBED, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_unsubscribe(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /unsubscribe \"team_uuid\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    _send_packet(CMD_UNSUBSCRIBE, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_use(const ParsedCommand &cmd)
{
    std::vector<uint8_t> payload;

    if (cmd.args.empty()) {
        payload.push_back(USE_NONE);
        _use_level = USE_NONE;
        _use_team_uuid.clear();
        _use_channel_uuid.clear();
        _use_thread_uuid.clear();
    } else if (cmd.args.size() == 1) {
        payload.push_back(USE_TEAM);
        pack_fixed(payload, cmd.args[0], 36);
        _use_level = USE_TEAM;
        _use_team_uuid = cmd.args[0];
        _use_channel_uuid.clear();
        _use_thread_uuid.clear();
    } else if (cmd.args.size() == 2) {
        payload.push_back(USE_CHANNEL);
        pack_fixed(payload, cmd.args[0], 36);
        pack_fixed(payload, cmd.args[1], 36);
        _use_level = USE_CHANNEL;
        _use_team_uuid    = cmd.args[0];
        _use_channel_uuid = cmd.args[1];
        _use_thread_uuid.clear();
    } else if (cmd.args.size() == 3) {
        payload.push_back(USE_THREAD);
        pack_fixed(payload, cmd.args[0], 36);
        pack_fixed(payload, cmd.args[1], 36);
        pack_fixed(payload, cmd.args[2], 36);
        _use_level = USE_THREAD;
        _use_team_uuid    = cmd.args[0];
        _use_channel_uuid = cmd.args[1];
        _use_thread_uuid  = cmd.args[2];
    } else {
        std::cerr << "Usage: /use [\"team_uuid\" [\"channel_uuid\" [\"thread_uuid\"]]]" << std::endl;
        return;
    }
    _send_packet(CMD_USE, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_create(const ParsedCommand &cmd)
{
    std::vector<uint8_t> payload;

    switch (_use_level) {
    case USE_NONE:
        if (cmd.args.size() != 2) {
            std::cerr << "Usage: /create \"team_name\" \"team_description\"" << std::endl;
            return;
        }
        pack_fixed(payload, cmd.args[0], 32);
        pack_fixed(payload, cmd.args[1], 255);
        break;
    case USE_TEAM:
        if (cmd.args.size() != 2) {
            std::cerr << "Usage: /create \"channel_name\" \"channel_description\"" << std::endl;
            return;
        }
        pack_fixed(payload, cmd.args[0], 32);
        pack_fixed(payload, cmd.args[1], 255);
        break;
    case USE_CHANNEL:
        if (cmd.args.size() != 2) {
            std::cerr << "Usage: /create \"thread_title\" \"thread_body\"" << std::endl;
            return;
        }
        pack_fixed(payload, cmd.args[0], 32);
        pack_fixed(payload, cmd.args[1], 512);
        break;
    case USE_THREAD:
        if (cmd.args.size() != 1) {
            std::cerr << "Usage: /create \"comment_body\"" << std::endl;
            return;
        }
        pack_fixed(payload, cmd.args[0], 512);
        break;
    }
    _send_packet(CMD_CREATE, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_list(const ParsedCommand &cmd)
{
    (void)cmd;
    _send_packet(CMD_LIST);
}

void Client::_cmd_info(const ParsedCommand &cmd)
{
    (void)cmd;
    _send_packet(CMD_INFO);
}

void Client::_handle_response(uint16_t code, const std::vector<uint8_t> &p)
{
    size_t off = 0;

    switch (code) {
    case RES_LOGIN_OK: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        _user_uuid = uuid;
        client_event_logged_in(uuid.c_str(), name.c_str());
        break;
    }
    case RES_LOGOUT_OK: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        client_event_logged_out(uuid.c_str(), name.c_str());
        _running = false;
        break;
    }
    case RES_USERS_LIST: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid   = read_fixed(p, off, 36);
            std::string name   = read_fixed(p, off, 32);
            uint8_t     status = read_u8(p, off);
            client_print_users(uuid.c_str(), name.c_str(), static_cast<int>(status));
        }
        break;
    }
    case RES_USER_INFO: {
        std::string uuid   = read_fixed(p, off, 36);
        std::string name   = read_fixed(p, off, 32);
        uint8_t     status = read_u8(p, off);
        client_print_user(uuid.c_str(), name.c_str(), static_cast<int>(status));
        break;
    }
    case RES_SEND_OK:
        break;
    case RES_MESSAGES_LIST: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string sender   = read_fixed(p, off, 36);
            /* receiver_uuid */    read_fixed(p, off, 36); // consumed but not needed for display
            int64_t     ts       = read_i64(p, off);
            std::string body     = read_fixed(p, off, 512);
            client_private_message_print_messages(sender.c_str(),
                static_cast<time_t>(ts), body.c_str());
        }
        break;
    }
    case RES_SUBSCRIBE_OK: {
        std::string team_uuid = read_fixed(p, off, 36);
        client_print_subscribed(_user_uuid.c_str(), team_uuid.c_str());
        break;
    }
    case RES_SUBSCRIBED_TEAMS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid = read_fixed(p, off, 36);
            std::string name = read_fixed(p, off, 32);
            std::string desc = read_fixed(p, off, 255);
            client_print_teams(uuid.c_str(), name.c_str(), desc.c_str());
        }
        break;
    }
    case RES_SUBSCRIBED_USERS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid   = read_fixed(p, off, 36);
            std::string name   = read_fixed(p, off, 32);
            uint8_t     status = read_u8(p, off);
            client_print_users(uuid.c_str(), name.c_str(), static_cast<int>(status));
        }
        break;
    }
    case RES_UNSUBSCRIBE_OK: {
        std::string team_uuid = read_fixed(p, off, 36);
        client_print_unsubscribed(_user_uuid.c_str(), team_uuid.c_str());
        break;
    }
    case RES_TEAM_CREATED: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_print_team_created(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case RES_CHANNEL_CREATED: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_print_channel_created(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case RES_THREAD_CREATED: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        int64_t     ts           = read_i64(p, off);
        std::string title        = read_fixed(p, off, 32);
        std::string body         = read_fixed(p, off, 512);
        client_print_thread_created(thread_uuid.c_str(), creator_uuid.c_str(),
            static_cast<time_t>(ts), title.c_str(), body.c_str());
        break;
    }
    case RES_REPLY_CREATED: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        int64_t     ts           = read_i64(p, off);
        std::string body         = read_fixed(p, off, 512);
        client_print_reply_created(thread_uuid.c_str(), creator_uuid.c_str(),
            static_cast<time_t>(ts), body.c_str());
        break;
    }
    case RES_LIST_TEAMS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid = read_fixed(p, off, 36);
            std::string name = read_fixed(p, off, 32);
            std::string desc = read_fixed(p, off, 255);
            client_print_teams(uuid.c_str(), name.c_str(), desc.c_str());
        }
        break;
    }
    case RES_LIST_CHANNELS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid = read_fixed(p, off, 36);
            std::string name = read_fixed(p, off, 32);
            std::string desc = read_fixed(p, off, 255);
            client_team_print_channels(uuid.c_str(), name.c_str(), desc.c_str());
        }
        break;
    }
    case RES_LIST_THREADS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string thread_uuid  = read_fixed(p, off, 36);
            std::string creator_uuid = read_fixed(p, off, 36);
            int64_t     ts           = read_i64(p, off);
            std::string title        = read_fixed(p, off, 32);
            std::string body         = read_fixed(p, off, 512);
            client_channel_print_threads(thread_uuid.c_str(), creator_uuid.c_str(),
                static_cast<time_t>(ts), title.c_str(), body.c_str());
        }
        break;
    }
    case RES_LIST_REPLIES: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string thread_uuid  = read_fixed(p, off, 36);
            std::string creator_uuid = read_fixed(p, off, 36);
            int64_t     ts           = read_i64(p, off);
            std::string body         = read_fixed(p, off, 512);
            client_thread_print_replies(thread_uuid.c_str(), creator_uuid.c_str(),
                static_cast<time_t>(ts), body.c_str());
        }
        break;
    }
    case RES_TEAM_INFO: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_print_team(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case RES_CHANNEL_INFO: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_print_channel(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case RES_THREAD_INFO: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        int64_t     ts           = read_i64(p, off);
        std::string title        = read_fixed(p, off, 32);
        std::string body         = read_fixed(p, off, 512);
        client_print_thread(thread_uuid.c_str(), creator_uuid.c_str(),
            static_cast<time_t>(ts), title.c_str(), body.c_str());
        break;
    }
    case RES_USER_DETAILS: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        client_print_user(uuid.c_str(), name.c_str(), -1);
        break;
    }

    case ERR_UNAUTHORIZED:
        client_error_unauthorized();
        break;
    case ERR_UNKNOWN_TEAM: {
        std::string uuid = read_fixed(p, off, 36);
        client_error_unknown_team(uuid.c_str());
        break;
    }
    case ERR_UNKNOWN_CHANNEL: {
        std::string uuid = read_fixed(p, off, 36);
        client_error_unknown_channel(uuid.c_str());
        break;
    }
    case ERR_UNKNOWN_THREAD: {
        std::string uuid = read_fixed(p, off, 36);
        client_error_unknown_thread(uuid.c_str());
        break;
    }
    case ERR_UNKNOWN_USER: {
        std::string uuid = read_fixed(p, off, 36);
        client_error_unknown_user(uuid.c_str());
        break;
    }
    case ERR_ALREADY_EXISTS:
        client_error_already_exist();
        break;
    case ERR_NOT_SUBSCRIBED:
        client_error_unauthorized();
        break;
    case ERR_INVALID_COMMAND:
        std::cerr << "Server: invalid command." << std::endl;
        break;

    case EVT_MESSAGE_RECEIVED: {
        std::string sender = read_fixed(p, off, 36);
        std::string body   = read_fixed(p, off, 512);
        client_event_private_message_received(sender.c_str(), body.c_str());
        break;
    }
    case EVT_TEAM_CREATED: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_event_team_created(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case EVT_CHANNEL_CREATED: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_event_channel_created(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case EVT_THREAD_CREATED: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        int64_t     ts           = read_i64(p, off);
        std::string title        = read_fixed(p, off, 32);
        std::string body         = read_fixed(p, off, 512);
        client_event_thread_created(thread_uuid.c_str(), creator_uuid.c_str(),
            static_cast<time_t>(ts), title.c_str(), body.c_str());
        break;
    }
    case EVT_REPLY_CREATED: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        /* timestamp */            read_i64(p, off);
        std::string body         = read_fixed(p, off, 512);
        client_event_thread_reply_received(
            _use_team_uuid.empty() ? "" : _use_team_uuid.c_str(),
            thread_uuid.c_str(), creator_uuid.c_str(), body.c_str());
        break;
    }
    case EVT_USER_LOGGED_IN: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        client_event_logged_in(uuid.c_str(), name.c_str());
        break;
    }
    case EVT_USER_LOGGED_OUT: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        client_event_logged_out(uuid.c_str(), name.c_str());
        break;
    }

    default:
        std::cerr << "Unknown response code: 0x" << std::hex << code << std::dec << std::endl;
        break;
    }
}
