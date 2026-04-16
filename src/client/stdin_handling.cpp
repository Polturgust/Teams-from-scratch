/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** stdin_handling.cpp
*/

#include "Client.hpp"

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
