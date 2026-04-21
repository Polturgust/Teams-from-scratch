/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Server.hpp
*/

#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <poll.h>
#include "../../include/data.hpp"
#include "../../include/protocole.hpp"
#include "../../include/mtp.hpp"

/** @brief Runtime socket state tracked for each connected client. */
struct ClientState {
    /** Client socket file descriptor. */
    int fd;
    /** Incremental receive buffer for framed protocol parsing. */
    std::vector<uint8_t> recv_buffer;
    /** Pending bytes queued for non-blocking writes. */
    std::vector<uint8_t> send_buffer;
    /** Context selected via /use command. */
    UseLevel use_level = USE_NONE;
    /** Selected team UUID for contextual operations. */
    std::string use_team_uuid;
    /** Selected channel UUID for contextual operations. */
    std::string use_channel_uuid;
    /** Selected thread UUID for contextual operations. */
    std::string use_thread_uuid;
};

/** @brief Poll-driven TCP server handling protocol framing and command dispatch. */
class Server {
public:
    /** @brief Create server listening configuration for a TCP port. */
    Server(int port);
    /** @brief Cleanly release server resources. */
    ~Server();
    /** @brief Start the main accept/read/write event loop. */
    void run();
    /** @brief Request graceful shutdown of the event loop. */
    void shutdown();
    /** Public shared server data manipulated by business handlers. */
    server_data_t data;
    /** Connected client states indexed by socket fd. */
    std::map<int, ClientState> clients;

private:
    int _server_fd;
    int _port;
    bool _running;
    mtp::Business _business;

    std::vector<struct pollfd> _build_pollfds();
    void _handle_new_connection();
    void _handle_client_read(int fd);
    void _handle_client_write(int fd);
    void _handle_client_disconnect(int fd);
    void _process_recv_buffer(int fd);
    void _dispatch(int fd, uint16_t cmd, const std::vector<uint8_t> &payload);
    void _send_result(int fd, const mtp::Result &result);
    void _queue_bytes(int fd, const std::vector<uint8_t> &bytes);
};

#endif
