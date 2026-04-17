/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_list.cpp
*/

#include "mtp.hpp"

#include "mtp_detail.hpp"

#include "protocole.hpp"

namespace mtp {

using namespace detail;

Result Business::handle_list(int fd)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const auto ctx_it = _data.client_contexts.find(fd);

    if (ctx_it == _data.client_contexts.end()) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const client_context_t &ctx = ctx_it->second;
    const uint8_t level = ctx.level;

    if (level == USE_NONE) {
        std::vector<uint8_t> payload;

        payload.reserve(4 + _data.teams.size() * (kUuidWireSize + kNameWireSize + kDescWireSize));
        append_u32(payload, static_cast<uint32_t>(_data.teams.size()));

        for (const auto &team : _data.teams) {
            append_uuid36(payload, team.uuid);
            append_name32(payload, team.name);
            append_fixed(payload, team.description, kDescWireSize);
        }

        res.response.code = RES_LIST_TEAMS;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    if (level == USE_TEAM) {
        team_t *team = find_team_by_uuid(_data, std::string_view(ctx.team_uuid, kUuidWireSize));
        if (!team) {
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::vector<uint8_t> payload;
        payload.reserve(4 + team->channels.size() * (kUuidWireSize + kNameWireSize + kDescWireSize));

        append_u32(payload, static_cast<uint32_t>(team->channels.size()));

        for (const auto &channel : team->channels) {
            append_uuid36(payload, channel.uuid);
            append_name32(payload, channel.name);
            append_fixed(payload, channel.description, kDescWireSize);
        }

        res.response.code = RES_LIST_CHANNELS;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    if (level == USE_CHANNEL) {
        team_t *team = find_team_by_uuid(_data, std::string_view(ctx.team_uuid, kUuidWireSize));
        if (!team) {
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        channel_t *channel = find_channel_by_uuid(*team, std::string_view(ctx.channel_uuid, kUuidWireSize));

        if (!channel) {
            res.response.code = ERR_UNKNOWN_CHANNEL;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::vector<uint8_t> payload;
        payload.reserve(4 + channel->threads.size() * (kUuidWireSize + kUuidWireSize + sizeof(uint64_t) + kNameWireSize + MAX_BODY_LENGTH));

        append_u32(payload, static_cast<uint32_t>(channel->threads.size()));

        for (const auto &thread : channel->threads) {
            append_uuid36(payload, thread.uuid);
            append_uuid36(payload, thread.creator_uuid);
            append_i64(payload, static_cast<int64_t>(thread.timestamp));
            append_name32(payload, thread.title);
            append_fixed(payload, thread.body, MAX_BODY_LENGTH);
        }

        res.response.code = RES_LIST_THREADS;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    if (level == USE_THREAD) {
        team_t *team = find_team_by_uuid(_data, std::string_view(ctx.team_uuid, kUuidWireSize));
        if (!team) {
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        channel_t *channel = find_channel_by_uuid(*team, std::string_view(ctx.channel_uuid, kUuidWireSize));

        if (!channel) {
            res.response.code = ERR_UNKNOWN_CHANNEL;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        thread_t *thread = find_thread_by_uuid(*channel, std::string_view(ctx.thread_uuid, kUuidWireSize));

        if (!thread) {
            res.response.code = ERR_UNKNOWN_THREAD;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::vector<uint8_t> payload;
        payload.reserve(4 + thread->replies.size() * (kUuidWireSize + kUuidWireSize + sizeof(uint64_t) + MAX_BODY_LENGTH));

        append_u32(payload, static_cast<uint32_t>(thread->replies.size()));

        for (const auto &reply : thread->replies) {
            append_fixed(payload, ctx.thread_uuid, kUuidWireSize);
            append_uuid36(payload, reply.creator_uuid);
            append_i64(payload, static_cast<int64_t>(reply.timestamp));
            append_fixed(payload, reply.body, MAX_BODY_LENGTH);
        }

        res.response.code = RES_LIST_REPLIES;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    res.response.code = ERR_INVALID_COMMAND;
    res.response.bytes = make_message(res.response.code, {});
    return res;
}

} // namespace mtp
