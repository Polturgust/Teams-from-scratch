/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** main.cpp
*/

#include <iostream>
#include <cstring>
#include <cstdlib>

static void print_help(void)
{
    std::cout << "USAGE: ./myteams_server port" << std::endl;
    std::cout << "\tport is the port number on which the server socket listens." << std::endl;
}

int main(int argc, char **argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }
    if (argc != 2) {
        print_help();
        return 84;
    }
    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number." << std::endl;
        return 84;
    }
    return 0;
}
