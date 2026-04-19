/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Client.hpp
*/

#ifndef CLIENT_HPP
    #define CLIENT_HPP

    #include <string>
    #include <vector>
    #include <cerrno>
    #include <cstring>
    #include <cstdlib>
    #include <cstdint>
    #include <iostream>
    #include <poll.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include "../../include/protocole.hpp"
    #include "../../libs/myteams/logging_client.h"
    #include "Helpers.hpp"

class Client {
    public:
        Client(const std::string &ip, int port);
        ~Client();
        void run();

    private:
        int _fd;
        bool _running;
        std::string  _user_uuid;

        std::vector<uint8_t> _recv_buf;
        std::vector<uint8_t> _send_buf;

        // Network
        void _connect(const std::string &ip, int port);
        void _handle_server_read();
        void _handle_server_write();
        void _handle_stdin();
        void _process_recv_buffer();
        void _send_packet(uint16_t cmd, const void *payload = nullptr, uint32_t size = 0);

        // Command parsing
        struct ParsedCommand {
            std::string name;
            std::vector<std::string> args;
            bool valid = true;
            std::string error;
        };
        ParsedCommand _parse_line(const std::string &line);
        void          _dispatch(const ParsedCommand &cmd);

        // Command handlers
        void _cmd_help();
        void _cmd_login(const ParsedCommand &cmd);
        void _cmd_logout(const ParsedCommand &cmd);
        void _cmd_users(const ParsedCommand &cmd);
        void _cmd_user(const ParsedCommand &cmd);
        void _cmd_send(const ParsedCommand &cmd);
        void _cmd_messages(const ParsedCommand &cmd);
        void _cmd_subscribe(const ParsedCommand &cmd);
        void _cmd_subscribed(const ParsedCommand &cmd);
        void _cmd_unsubscribe(const ParsedCommand &cmd);
        void _cmd_use(const ParsedCommand &cmd);
        void _cmd_create(const ParsedCommand &cmd);
        void _cmd_list(const ParsedCommand &cmd);
        void _cmd_info(const ParsedCommand &cmd);

        // Response handler
        void _handle_response(uint16_t code, const std::vector<uint8_t> &payload);

        UseLevel    _use_level = USE_NONE;
        std::string _use_team_uuid;
        std::string _use_channel_uuid;
        std::string _use_thread_uuid;
        std::string _stdin_buf;
};

#endif // CLIENT_HPP