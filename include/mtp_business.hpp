/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_business.hpp
*/

#ifndef MTP_BUSINESS_HPP
#define MTP_BUSINESS_HPP

#include <cstdint>
#include <string_view>
#include <vector>

#include "data.hpp"

namespace mtp {

struct Packet {
    uint16_t code;
    std::vector<uint8_t> bytes;
};

struct Push {
    Packet packet;
    std::vector<int> fds;
};

struct Result {
    Packet response;
    std::vector<Push> pushes;
};

class Business {
public:
    explicit Business(server_data_t &data);

    Result handle_login(int fd, std::string_view name);
    Result handle_logout(int fd);

    Result handle_users(int fd);
    Result handle_user(int fd, std::string_view user_uuid);

    Result handle_send(int fd, std::string_view receiver_uuid, std::string_view body);
    Result handle_messages(int fd, std::string_view user_uuid);

private:
    server_data_t &_data;
};

} // namespace mtp

#endif
