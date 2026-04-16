/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_info.cpp
*/

#include "mtp.hpp"

#include <cstring>

#include "mtp_detail.hpp"

#include "protocole.hpp"

namespace mtp {

using namespace detail;

Result Business::handle_info(int fd)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string user_uuid = it->second;
    const auto ctx_it = _data.client_contexts.find(fd);
    
    if (ctx_it == _data.client_contexts.end()) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const client_context_t &ctx = ctx_it->second;
    const uint8_t level = ctx.level;

    // INFO_USER: return current user details
    if (level == USE_NONE) {
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

    // INFO_TEAM: return current team details
    if (level == USE_TEAM) {
        team_t *team = find_team_by_uuid(_data, std::string_view(ctx.team_uuid, kUuidWireSize));
        if (!team) {
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kNameWireSize + kDescWireSize);
        append_uuid36(payload, team->uuid);
        append_name32(payload, team->name);
        append_fixed(payload, team->description, kDescWireSize);

        res.response.code = RES_TEAM_INFO;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // INFO_CHANNEL: return current channel details
    if (level == USE_CHANNEL) {
        team_t *team = find_team_by_uuid(_data, std::string_view(ctx.team_uuid, kUuidWireSize));
        if (!team) {
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        channel_t *channel = nullptr;
        for (auto &c : team->channels) {
            if (std::string_view(c.uuid, kUuidWireSize) == std::string_view(ctx.channel_uuid, kUuidWireSize)) {
                channel = &c;
                break;
            }
        }

        if (!channel) {
            res.response.code = ERR_UNKNOWN_CHANNEL;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kNameWireSize + kDescWireSize);
        append_uuid36(payload, channel->uuid);
        append_name32(payload, channel->name);
        append_fixed(payload, channel->description, kDescWireSize);

        res.response.code = RES_CHANNEL_INFO;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // INFO_THREAD: return current thread details
    if (level == USE_THREAD) {
        team_t *team = find_team_by_uuid(_data, std::string_view(ctx.team_uuid, kUuidWireSize));
        if (!team) {
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        channel_t *channel = nullptr;
        for (auto &c : team->channels) {
            if (std::string_view(c.uuid, kUuidWireSize) == std::string_view(ctx.channel_uuid, kUuidWireSize)) {
                channel = &c;
                break;
            }
        }

        if (!channel) {
            res.response.code = ERR_UNKNOWN_CHANNEL;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        thread_t *thread = nullptr;
        for (auto &t : channel->threads) {
            if (std::string_view(t.uuid, kUuidWireSize) == std::string_view(ctx.thread_uuid, kUuidWireSize)) {
                thread = &t;
                break;
            }
        }

        if (!thread) {
            res.response.code = ERR_UNKNOWN_THREAD;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kUuidWireSize + kNameWireSize + MAX_BODY_LENGTH + sizeof(uint32_t));
        append_uuid36(payload, thread->uuid);
        append_uuid36(payload, thread->creator_uuid);
        append_name32(payload, thread->title);
        append_fixed(payload, thread->body, MAX_BODY_LENGTH);
        append_u32(payload, static_cast<uint32_t>(thread->timestamp));

        res.response.code = RES_THREAD_INFO;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    res.response.code = ERR_INVALID_COMMAND;
    res.response.bytes = make_message(res.response.code, {});
    return res;
}

} // namespace mtp
