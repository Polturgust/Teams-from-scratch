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

struct Result {
    Packet response;
    std::vector<Packet> pushes;
};

class Business {
public:
    explicit Business(server_data_t &data);

    Result handle_login(int fd, std::string_view name);
    Result handle_users(int fd);

private:
    server_data_t &_data;
};

} // namespace mtp

#endif
