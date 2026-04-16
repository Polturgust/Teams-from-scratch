/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** main.cpp
*/

#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <csignal>

static Server *g_server = nullptr;

static void sigint_handler(int sig)
{
    (void)sig;
    if (g_server)
        g_server->shutdown();
}

static void print_help()
{
    std::cout << "USAGE: ./myteams_server port" << std::endl;
    std::cout << "\tport is the port number on which the "
              << "server socket listens." << std::endl;
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
        std::cout << "Invalid port number." << std::endl;
        return 84;
    }
    try {
        Server server(port);
        g_server = &server;

        struct sigaction sa = {};
        sa.sa_handler = sigint_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
        server.run();

    } catch (const std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 84;
    }
    return 0;
}