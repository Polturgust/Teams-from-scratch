/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_users.cpp
*/

#include "mtp.hpp"

#include "mtp_detail.hpp"

#include "protocole.hpp"

namespace mtp {

using namespace detail;

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

Result Business::handle_user(int fd, std::string_view user_uuid)
{
    Result res;

    if (!is_logged_in(_data, fd)) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    if (!is_uuid36(user_uuid)) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    user_t *user = find_user_by_uuid(_data, user_uuid);

    if (!user) {
        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize);
        append_fixed(payload, user_uuid.data(), kUuidWireSize);
        res.response.code = ERR_UNKNOWN_USER;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // RES_USER_INFO: uuid(36) + name(32) + status(1)
    std::vector<uint8_t> payload;
    payload.reserve(kUuidWireSize + kNameWireSize + 1);
    append_uuid36(payload, user->uuid);
    append_name32(payload, user->name);
    const uint8_t status = is_user_online(_data, *user) ? 0x01 : 0x00;
    append_fixed(payload, &status, sizeof(status));

    res.response.code = RES_USER_INFO;
    res.response.bytes = make_message(res.response.code, payload);
    return res;
}

} // namespace mtp
