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
#include <ctime>

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

        // EVT_USER_LOGGED_IN: uuid(36) + name(32)
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

    // RES_LOGOUT_OK: uuid(36) + name(32)
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
        evt.fds.push_back(fd); // the session that logged out should receive the event too
        res.pushes.push_back(std::move(evt));
    }

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

Result Business::handle_user(int fd, std::string_view user_uuid)
{
    Result res;

    if (!is_logged_in(_data, fd)) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    user_t *user = nullptr;
    for (auto &u : _data.users) {
        if (std::string_view(u.uuid, kUuidWireSize) == user_uuid) {
            user = &u;
            break;
        }
    }

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

Result Business::handle_send(int fd, std::string_view receiver_uuid, std::string_view body)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string sender_uuid = it->second;

    user_t *receiver = nullptr;
    for (auto &u : _data.users) {
        if (std::string_view(u.uuid, kUuidWireSize) == receiver_uuid) {
            receiver = &u;
            break;
        }
    }

    if (!receiver) {
        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize);
        append_fixed(payload, receiver_uuid.data(), kUuidWireSize);
        res.response.code = ERR_UNKNOWN_USER;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // Persist message
    message_t msg{};
    std::memset(&msg, 0, sizeof(msg));
    std::memcpy(msg.sender_uuid, sender_uuid.data(), std::min<std::size_t>(sender_uuid.size(), kUuidWireSize));
    std::memcpy(msg.receiver_uuid, receiver_uuid.data(), kUuidWireSize);
    msg.sender_uuid[kUuidWireSize] = '\0';
    msg.receiver_uuid[kUuidWireSize] = '\0';

    body = trim_padded_name(body);
    const std::size_t copy_len = std::min<std::size_t>(body.size(), MAX_BODY_LENGTH);
    std::memcpy(msg.body, body.data(), copy_len);
    msg.timestamp = std::time(nullptr);

    _data.messages.push_back(msg);

    server_event_private_message_sended(sender_uuid.c_str(), receiver->uuid, msg.body);

    // RES_SEND_OK: empty
    res.response.code = RES_SEND_OK;
    res.response.bytes = make_message(res.response.code, {});

    // EVT_MESSAGE_RECEIVED: sender_uuid(36) + body(512)
    std::vector<uint8_t> evt_payload;
    evt_payload.reserve(kUuidWireSize + MAX_BODY_LENGTH);
    append_fixed(evt_payload, sender_uuid.data(), kUuidWireSize);
    append_fixed(evt_payload, msg.body, MAX_BODY_LENGTH);

    Push push;
    push.packet.code = EVT_MESSAGE_RECEIVED;
    push.packet.bytes = make_message(push.packet.code, evt_payload);

    for (const auto &kv : _data.sessions) {
        if (std::string_view(kv.second) == std::string_view(receiver->uuid, kUuidWireSize))
            push.fds.push_back(kv.first);
    }

    if (!push.fds.empty())
        res.pushes.push_back(std::move(push));

    return res;
}

Result Business::handle_messages(int fd, std::string_view user_uuid)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string self_uuid = it->second;

    bool exists = false;
    for (const auto &u : _data.users) {
        if (std::string_view(u.uuid, kUuidWireSize) == user_uuid) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize);
        append_fixed(payload, user_uuid.data(), kUuidWireSize);
        res.response.code = ERR_UNKNOWN_USER;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // RES_MESSAGES_LIST: count(4) + [sender_uuid(36) + receiver_uuid(36) + timestamp(8) + body(512)]*
    std::vector<const message_t *> msgs;
    for (const auto &m : _data.messages) {
        const std::string_view sender(m.sender_uuid, kUuidWireSize);
        const std::string_view receiver(m.receiver_uuid, kUuidWireSize);
        const bool match =
            (sender == self_uuid && receiver == user_uuid) ||
            (sender == user_uuid && receiver == self_uuid);
        if (match)
            msgs.push_back(&m);
    }

    std::vector<uint8_t> payload;
    append_u32(payload, static_cast<uint32_t>(msgs.size()));

    auto htonll = [](uint64_t v) -> uint64_t {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        return (static_cast<uint64_t>(htonl(static_cast<uint32_t>(v & 0xFFFFFFFFULL))) << 32) |
            htonl(static_cast<uint32_t>(v >> 32));
#else
        return v;
#endif
    };

    for (const auto *m : msgs) {
        append_fixed(payload, m->sender_uuid, kUuidWireSize);
        append_fixed(payload, m->receiver_uuid, kUuidWireSize);

        const int64_t ts = static_cast<int64_t>(m->timestamp);
        const uint64_t ts_be = htonll(static_cast<uint64_t>(ts));
        append_fixed(payload, &ts_be, sizeof(ts_be));

        append_fixed(payload, m->body, MAX_BODY_LENGTH);
    }

    res.response.code = RES_MESSAGES_LIST;
    res.response.bytes = make_message(res.response.code, payload);
    return res;
}

} // namespace mtp
