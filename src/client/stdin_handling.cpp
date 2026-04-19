/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** stdin_handling.cpp
*/

#include "Client.hpp"

void Client::_handle_stdin()
{
    char buf[4096];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) {
        _running = false;
        return;
    }

    _stdin_buf.append(buf, n);

    size_t pos;
    while ((pos = _stdin_buf.find('\n')) != std::string::npos) {
        std::string line = _stdin_buf.substr(0, pos);
        _stdin_buf.erase(0, pos + 1);

        if (line.empty())
            continue;

        ParsedCommand cmd = _parse_line(line);
        if (!cmd.valid) {
            std::cout << "Parse error: " << cmd.error << std::endl;
            continue;
        }
        _dispatch(cmd);
    }
}