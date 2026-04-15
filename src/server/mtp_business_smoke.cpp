/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_business_smoke.cpp
*/

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>

#include <arpa/inet.h>

#include "mtp_business.hpp"
#include "protocole.hpp"

namespace {

struct Header {
    uint16_t code;
    uint32_t payload_size;
};

static Header parse_header(const std::vector<uint8_t> &bytes)
{
    assert(bytes.size() >= 6);

    uint16_t code_be;
    uint32_t size_be;
    std::memcpy(&code_be, bytes.data(), sizeof(code_be));
    std::memcpy(&size_be, bytes.data() + sizeof(code_be), sizeof(size_be));

    Header h;
    h.code = ntohs(code_be);
    h.payload_size = ntohl(size_be);
    return h;
}

static uint32_t read_u32_be(const uint8_t *p)
{
    uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return ntohl(v);
}

} // namespace

int main()
{
    server_data_t data;
    mtp::Business biz(data);

    {
        auto r = biz.handle_users(10);
        const auto h = parse_header(r.response.bytes);
        assert(h.code == ERR_UNAUTHORIZED);
        assert(h.payload_size == 0);
        assert(r.response.bytes.size() == 6);
    }

    {
        auto r = biz.handle_login(10, std::string_view("Hilal"));
        const auto h = parse_header(r.response.bytes);
        assert(h.code == RES_LOGIN_OK);
        assert(h.payload_size == 36 + 32);
        assert(r.response.bytes.size() == 6 + h.payload_size);
        assert(r.pushes.size() == 1);

        const auto he = parse_header(r.pushes[0].bytes);
        assert(he.code == EVT_USER_LOGGED_IN);
        assert(he.payload_size == h.payload_size);
        assert(r.pushes[0].bytes.size() == 6 + he.payload_size);

        assert(data.users.size() == 1);
        assert(data.sessions.size() == 1);
    }

    {
        auto r = biz.handle_users(10);
        const auto h = parse_header(r.response.bytes);
        assert(h.code == RES_USERS_LIST);
        assert(h.payload_size == 4 + (36 + 32 + 1) * 1);
        assert(r.response.bytes.size() == 6 + h.payload_size);

        const uint8_t *payload = r.response.bytes.data() + 6;
        const uint32_t count = read_u32_be(payload);
        assert(count == 1);

        const uint8_t status = payload[4 + 36 + 32];
        assert(status == 0x01);
    }

    {
        // Second connection logs in with same name: should not create a second user.
        auto r = biz.handle_login(11, std::string_view("Hilal"));
        (void)r;
        assert(data.users.size() == 1);
        assert(data.sessions.size() == 2);

        auto r_users_fd10 = biz.handle_users(10);
        auto r_users_fd11 = biz.handle_users(11);
        assert(parse_header(r_users_fd10.response.bytes).code == RES_USERS_LIST);
        assert(parse_header(r_users_fd11.response.bytes).code == RES_USERS_LIST);
    }

    std::cout << "mtp_business_smoke: OK" << std::endl;
    return 0;
}
