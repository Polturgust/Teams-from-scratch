/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_auth.cpp
*/

#include "mtp.hpp"

#include "mtp_detail.hpp"

#include "logging_server.h"
#include "protocole.hpp"

namespace mtp {

using namespace detail;

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
    }

    const bool was_online = is_user_online(_data, *user);

    user->is_logged = true;
    user->fd = fd;
    _data.sessions[fd] = std::string(user->uuid);

    // RES_LOGIN_OK: uuid(36) + name(32)
    std::vector<uint8_t> payload;
    payload.reserve(kUuidWireSize + kNameWireSize);
    append_uuid36(payload, user->uuid);
    append_name32(payload, user->name);

    res.response.code = RES_LOGIN_OK;
    res.response.bytes = make_message(res.response.code, payload);

    if (!was_online) {
        server_event_user_logged_in(user->uuid);

        Push evt;
        evt.packet.code = EVT_USER_LOGGED_IN;
        evt.packet.bytes = make_message(evt.packet.code, payload);
        evt.fds.reserve(_data.sessions.size());
        for (const auto &kv : _data.sessions)
            evt.fds.push_back(kv.first);
        res.pushes.push_back(std::move(evt));
    }

    return res;
}

Result Business::handle_logout(int fd)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string uuid = it->second;

    user_t *user = nullptr;
    for (auto &u : _data.users) {
        if (std::string_view(u.uuid, kUuidWireSize) == uuid) {
            user = &u;
            break;
        }
    }

    std::vector<uint8_t> payload;
    payload.reserve(kUuidWireSize + kNameWireSize);
    append_fixed(payload, uuid.data(), kUuidWireSize);
    if (user)
        append_name32(payload, user->name);
    else {
        char zeros[kNameWireSize];
        std::memset(zeros, 0, sizeof(zeros));
        append_fixed(payload, zeros, sizeof(zeros));
    }

    res.response.code = RES_LOGOUT_OK;
    res.response.bytes = make_message(res.response.code, payload);

    _data.sessions.erase(it);

    bool still_online = false;
    for (const auto &kv : _data.sessions) {
        if (kv.second == uuid) {
            still_online = true;
            break;
        }
    }

    if (user)
        user->is_logged = still_online;

    if (!still_online) {
        server_event_user_logged_out(uuid.c_str());

        Push evt;
        evt.packet.code = EVT_USER_LOGGED_OUT;
        evt.packet.bytes = make_message(evt.packet.code, payload);
        evt.fds.reserve(_data.sessions.size() + 1);
        for (const auto &kv : _data.sessions)
            evt.fds.push_back(kv.first);
        evt.fds.push_back(fd);
        res.pushes.push_back(std::move(evt));
    }

    return res;
}

} // namespace mtp
