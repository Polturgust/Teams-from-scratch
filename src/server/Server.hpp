/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Server.hpp
*/

#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <poll.h>
#include "../../include/data.hpp"
#include "../../include/protocole.hpp"

struct ClientState {
    int fd;
    std::vector<uint8_t> recv_buffer;
    std::vector<uint8_t> send_buffer;
    std::string user_uuid;
    UseLevel use_level = USE_NONE;
    std::string use_team_uuid;
    std::string use_channel_uuid;
    std::string use_thread_uuid;
};

class Server {
    public:
        Server(int port);
        ~Server();
        void run();
        void shutdown();
        void queue_response(int fd, uint16_t cmd, const void *data = nullptr, uint32_t size = 0);
        void broadcast_logged(uint16_t cmd, const void *data, uint32_t size, int exclude_fd = -1);
        void broadcast_team(const std::string &team_uuid, uint16_t cmd, const void *data, uint32_t size, int exclude_fd = -1);
        server_data_t data;
        std::map<int, ClientState> clients;

    private:
        int _server_fd;
        int _port;
        bool _running;
        std::vector<struct pollfd> _build_pollfds();
        void _handle_new_connection();
        void _handle_client_read(int fd);
        void _handle_client_write(int fd);
        void _handle_client_disconnect(int fd);
        void _process_recv_buffer(int fd);
        void _dispatch(int fd, uint16_t cmd, const std::vector<uint8_t> &payload);
};