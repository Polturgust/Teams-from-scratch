/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_messages.cpp
*/

#include "mtp.hpp"

#include <ctime>

#include "mtp_detail.hpp"

#include "logging_server.h"
#include "protocole.hpp"

namespace mtp {

using namespace detail;

Result Business::handle_send(int fd, std::string_view receiver_uuid, std::string_view body)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    if (!is_uuid36(receiver_uuid)) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string sender_uuid = it->second;

    user_t *receiver = find_user_by_uuid(_data, receiver_uuid);

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

    if (!is_uuid36(user_uuid)) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string self_uuid = it->second;

    const bool exists = (find_user_by_uuid(_data, user_uuid) != nullptr);

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
