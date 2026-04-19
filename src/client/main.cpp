/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description: main for client part
** main.cpp
*/

#include <iostream>
#include <cstring>
#include <cstdlib>
#include "../client/Client.hpp"

static void print_help(void)
{
    std::cout << "USAGE: ./myteams_cli ip port" << std::endl;
    std::cout << "\tip is the server ip address on which the server socket listens" << std::endl;
    std::cout << "\tport is the port number on which the server socket listens" << std::endl;
}

int main(int argc, char **argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }
    if (argc != 3) {
        print_help();
        return 84;
    }
    int port = std::atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        std::cout << "Invalid port number." << std::endl;
        return 84;
    }
    Client client(argv[1], port);
    client.run();
    return 0;
}