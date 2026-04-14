/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** protocole.hpp
*/

#ifndef PROTOCOLE_HPP
#define PROTOCOLE_HPP

    #include <cstdint>
    #include <arpa/inet.h>

#define HEADER_SIZE 6

enum Command : uint16_t {
    CMD_LOGIN = 0x01,
    CMD_LOGOUT = 0x02,
    CMD_USERS = 0x03,
    CMD_USER = 0x04,
    CMD_SEND = 0x05,
    CMD_MESSAGES = 0x06,
    CMD_SUBSCRIBE = 0x07,
    CMD_SUBSCRIBED = 0x08,
    CMD_UNSUBSCRIBE = 0x09,
    CMD_USE = 0x0A,
    CMD_CREATE  = 0x0B,
    CMD_LIST = 0x0C,
    CMD_INFO = 0x0D,
};

#endif