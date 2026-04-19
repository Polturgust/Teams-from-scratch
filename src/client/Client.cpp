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
            std::cout << "poll: " << strerror(errno) << std::endl;
            break;
        }

        if (fds[0].revents & POLLIN)
            _handle_stdin();

        if (fds[1].revents & POLLOUT)
            _handle_server_write();

        if (fds[1].revents & POLLIN)
            _handle_server_read();

        if (fds[1].revents & (POLLHUP | POLLERR)) {
            std::cout << "Server disconnected." << std::endl;
            _running = false;
        }
    }
}

