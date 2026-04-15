/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_business.cpp
*/

#include "mtp_business.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include <arpa/inet.h>
#include <uuid/uuid.h>

#include "logging_server.h"
#include "protocole.hpp"

namespace {

constexpr std::size_t kHeaderSize = 6;
constexpr std::size_t kUuidWireSize = 36;
constexpr std::size_t kNameWireSize = 32;

static std::vector<uint8_t> make_message(uint16_t code, const std::vector<uint8_t> &payload)
{
    std::vector<uint8_t> out;
    out.reserve(kHeaderSize + payload.size());

    const uint16_t cmd_be = htons(code);
    const uint32_t size_be = htonl(static_cast<uint32_t>(payload.size()));

    const uint8_t *cmd_ptr = reinterpret_cast<const uint8_t *>(&cmd_be);
    const uint8_t *size_ptr = reinterpret_cast<const uint8_t *>(&size_be);

    out.insert(out.end(), cmd_ptr, cmd_ptr + sizeof(cmd_be));
    out.insert(out.end(), size_ptr, size_ptr + sizeof(size_be));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

static void append_fixed(std::vector<uint8_t> &out, const void *data, std::size_t size)
{
    const uint8_t *ptr = static_cast<const uint8_t *>(data);
    out.insert(out.end(), ptr, ptr + size);
}

static void append_u32(std::vector<uint8_t> &out, uint32_t value)
{
    const uint32_t be = htonl(value);
    append_fixed(out, &be, sizeof(be));
}

static void append_uuid36(std::vector<uint8_t> &out, const char *uuid37)
{
    // Internal UUID strings are null-terminated (37). Wire uses exactly 36 bytes, no '\0'.
    append_fixed(out, uuid37, kUuidWireSize);
}

static void append_name32(std::vector<uint8_t> &out, const char name32[kNameWireSize])
{
    append_fixed(out, name32, kNameWireSize);
}

static std::string_view trim_padded_name(std::string_view name)
{
    const auto null_pos = name.find('\0');
    if (null_pos != std::string_view::npos)
        return name.substr(0, null_pos);
    return name;
}

static user_t *find_user_by_name(server_data_t &data, std::string_view name)
{
    for (auto &user : data.users) {
        const std::string_view uname(user.name, MAX_NAME_LENGTH);
        if (trim_padded_name(uname) == name)
            return &user;
    }
    return nullptr;
}

static void set_fixed_name(char out_name32[MAX_NAME_LENGTH], std::string_view name)
{
    std::memset(out_name32, 0, MAX_NAME_LENGTH);
    const std::size_t copy_len = std::min<std::size_t>(name.size(), MAX_NAME_LENGTH);
    std::memcpy(out_name32, name.data(), copy_len);
}

static void new_uuid(char out_uuid37[UUID_STR_LEN])
{
    uuid_t bin;
    uuid_generate(bin);
    uuid_unparse(bin, out_uuid37);
}

static bool is_logged_in(const server_data_t &data, int fd)
{
    return data.sessions.find(fd) != data.sessions.end();
}

static bool is_user_online(const server_data_t &data, const user_t &user)
{
    const std::string uuid(user.uuid);
    for (const auto &kv : data.sessions) {
        if (kv.second == uuid)
            return true;
    }
    return false;
}

} // namespace

namespace mtp {

Business::Business(server_data_t &data)
    : _data(data)
{
}

Result Business::handle_login(int fd, std::string_view name)
{
    Result res;

    name = trim_padded_name(name);

    user_t *user = find_user_by_name(_data, name);
    if (!user) {
        user_t created{};
        std::memset(&created, 0, sizeof(created));
        new_uuid(created.uuid);
        set_fixed_name(created.name, name);
        created.is_logged = true;
        created.fd = fd;
        _data.users.push_back(created);
        user = &_data.users.back();

        server_event_user_created(user->uuid, user->name);
    } else {
        user->is_logged = true;
        user->fd = fd;
    }

    _data.sessions[fd] = std::string(user->uuid);

    server_event_user_logged_in(user->uuid);

    // RES_LOGIN_OK: uuid(36) + name(32)
    std::vector<uint8_t> payload;
    payload.reserve(kUuidWireSize + kNameWireSize);
    append_uuid36(payload, user->uuid);
    append_name32(payload, user->name);

    res.response.code = RES_LOGIN_OK;
    res.response.bytes = make_message(res.response.code, payload);

    // EVT_USER_LOGGED_IN: uuid(36) + name(32)
    Packet evt;
    evt.code = EVT_USER_LOGGED_IN;
    evt.bytes = make_message(evt.code, payload);
    res.pushes.push_back(std::move(evt));

    return res;
}

Result Business::handle_users(int fd)
{
    Result res;

    if (!is_logged_in(_data, fd)) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    // RES_USERS_LIST: count(4) + [uuid(36) + name(32) + status(1)]*
    std::vector<uint8_t> payload;

    append_u32(payload, static_cast<uint32_t>(_data.users.size()));
    for (const auto &u : _data.users) {
        append_uuid36(payload, u.uuid);
        append_name32(payload, u.name);
        const uint8_t status = is_user_online(_data, u) ? 0x01 : 0x00;
        append_fixed(payload, &status, sizeof(status));
    }

    res.response.code = RES_USERS_LIST;
    res.response.bytes = make_message(res.response.code, payload);
    return res;
}

} // namespace mtp
