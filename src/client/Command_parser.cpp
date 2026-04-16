/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Command_parser.cpp
*/

#include "Client.hpp"

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

