/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_context.cpp
*/

#include "mtp.hpp"

#include <cstring>

#include "mtp_detail.hpp"

#include "protocole.hpp"

namespace mtp {

using namespace detail;

Result Business::handle_use(int fd, std::string_view payload_bytes)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    if (payload_bytes.empty()) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const uint8_t level = static_cast<uint8_t>(payload_bytes[0]);
    const std::string user_uuid = it->second;

    client_context_t ctx{};
    ctx.level = level;

    if (level == USE_NONE) {
        _data.client_contexts[fd] = ctx;

        user_t *user = find_user_by_uuid(_data, user_uuid);
        std::vector<uint8_t> payload;
        if (user) {
            payload.reserve(kUuidWireSize + kNameWireSize);
            append_uuid36(payload, user->uuid);
            append_name32(payload, user->name);
        } else {
            payload.reserve(kUuidWireSize + kNameWireSize);
            append_fixed(payload, user_uuid.data(), kUuidWireSize);
            char zeros[kNameWireSize];
            std::memset(zeros, 0, sizeof(zeros));
            append_fixed(payload, zeros, sizeof(zeros));
        }

        res.response.code = RES_USER_DETAILS;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    if (level == USE_TEAM) {
        if (payload_bytes.size() < 1 + kUuidWireSize) {
            res.response.code = ERR_INVALID_COMMAND;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::string_view team_uuid(payload_bytes.data() + 1, kUuidWireSize);
        team_t *team = find_team_by_uuid(_data, team_uuid);
        if (!team) {
            std::vector<uint8_t> payload;
            payload.reserve(kUuidWireSize);
            append_fixed(payload, team_uuid.data(), kUuidWireSize);
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, payload);
            return res;
        }

        if (!is_subscribed_to(*team, user_uuid)) {
            res.response.code = ERR_NOT_SUBSCRIBED;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::memcpy(ctx.team_uuid, team_uuid.data(), kUuidWireSize);
        _data.client_contexts[fd] = ctx;

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kNameWireSize + kDescWireSize);
        append_uuid36(payload, team->uuid);
        append_name32(payload, team->name);
        append_fixed(payload, team->description, kDescWireSize);

        res.response.code = RES_TEAM_INFO;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    if (level == USE_CHANNEL) {
        if (payload_bytes.size() < 1 + kUuidWireSize + kUuidWireSize) {
            res.response.code = ERR_INVALID_COMMAND;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::string_view team_uuid(payload_bytes.data() + 1, kUuidWireSize);
        std::string_view channel_uuid(payload_bytes.data() + 1 + kUuidWireSize, kUuidWireSize);

        team_t *team = find_team_by_uuid(_data, team_uuid);
        if (!team) {
            std::vector<uint8_t> payload;
            payload.reserve(kUuidWireSize);
            append_fixed(payload, team_uuid.data(), kUuidWireSize);
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, payload);
            return res;
        }

        if (!is_subscribed_to(*team, user_uuid)) {
            res.response.code = ERR_NOT_SUBSCRIBED;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        channel_t *channel = find_channel_by_uuid(*team, channel_uuid);

        if (!channel) {
            std::vector<uint8_t> payload;
            payload.reserve(kUuidWireSize);
            append_fixed(payload, channel_uuid.data(), kUuidWireSize);
            res.response.code = ERR_UNKNOWN_CHANNEL;
            res.response.bytes = make_message(res.response.code, payload);
            return res;
        }

        std::memcpy(ctx.team_uuid, team_uuid.data(), kUuidWireSize);
        std::memcpy(ctx.channel_uuid, channel_uuid.data(), kUuidWireSize);
        _data.client_contexts[fd] = ctx;

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kNameWireSize + kDescWireSize);
        append_uuid36(payload, channel->uuid);
        append_name32(payload, channel->name);
        append_fixed(payload, channel->description, kDescWireSize);

        res.response.code = RES_CHANNEL_INFO;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    if (level == USE_THREAD) {
        if (payload_bytes.size() < 1 + kUuidWireSize + kUuidWireSize + kUuidWireSize) {
            res.response.code = ERR_INVALID_COMMAND;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::string_view team_uuid(payload_bytes.data() + 1, kUuidWireSize);
        std::string_view channel_uuid(payload_bytes.data() + 1 + kUuidWireSize, kUuidWireSize);
        std::string_view thread_uuid(payload_bytes.data() + 1 + kUuidWireSize + kUuidWireSize, kUuidWireSize);

        team_t *team = find_team_by_uuid(_data, team_uuid);
        if (!team) {
            std::vector<uint8_t> payload;
            payload.reserve(kUuidWireSize);
            append_fixed(payload, team_uuid.data(), kUuidWireSize);
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, payload);
            return res;
        }

        if (!is_subscribed_to(*team, user_uuid)) {
            res.response.code = ERR_NOT_SUBSCRIBED;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        channel_t *channel = find_channel_by_uuid(*team, channel_uuid);

        if (!channel) {
            std::vector<uint8_t> payload;
            payload.reserve(kUuidWireSize);
            append_fixed(payload, channel_uuid.data(), kUuidWireSize);
            res.response.code = ERR_UNKNOWN_CHANNEL;
            res.response.bytes = make_message(res.response.code, payload);
            return res;
        }

        thread_t *thread = find_thread_by_uuid(*channel, thread_uuid);

        if (!thread) {
            std::vector<uint8_t> payload;
            payload.reserve(kUuidWireSize);
            append_fixed(payload, thread_uuid.data(), kUuidWireSize);
            res.response.code = ERR_UNKNOWN_THREAD;
            res.response.bytes = make_message(res.response.code, payload);
            return res;
        }

        std::memcpy(ctx.team_uuid, team_uuid.data(), kUuidWireSize);
        std::memcpy(ctx.channel_uuid, channel_uuid.data(), kUuidWireSize);
        std::memcpy(ctx.thread_uuid, thread_uuid.data(), kUuidWireSize);
        _data.client_contexts[fd] = ctx;

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kUuidWireSize + 8 + kNameWireSize + MAX_BODY_LENGTH);
        append_uuid36(payload, thread->uuid);
        append_uuid36(payload, thread->creator_uuid);
        append_i64(payload, static_cast<int64_t>(thread->timestamp));

        append_fixed(payload, thread->title, kNameWireSize);
        append_fixed(payload, thread->body, MAX_BODY_LENGTH);

        res.response.code = RES_THREAD_INFO;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    res.response.code = ERR_INVALID_COMMAND;
    res.response.bytes = make_message(res.response.code, {});
    return res;
}

} // namespace mtp
