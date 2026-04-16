/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_list.cpp
*/

#include "mtp.hpp"

#include <cstring>

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

    const std::string user_uuid = it->second;
    const auto ctx_it = _data.client_contexts.find(fd);
    
    if (ctx_it == _data.client_contexts.end()) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const client_context_t &ctx = ctx_it->second;
    const uint8_t level = ctx.level;

    // LIST_TEAMS: list all teams user is subscribed to
    if (level == USE_NONE) {
        std::vector<uint8_t> payload;
        std::vector<team_t *> subscribed_teams;

        // Find all teams the user is subscribed to
        for (auto &team : _data.teams) {
            if (is_subscribed_to(team, user_uuid)) {
                subscribed_teams.push_back(&team);
            }
        }

        // Count header
        payload.reserve(4 + subscribed_teams.size() * (kUuidWireSize + kNameWireSize + kDescWireSize));
        append_u32(payload, static_cast<uint32_t>(subscribed_teams.size()));

        // Each team: uuid(36) + name(32) + desc(255)
        for (const auto *team : subscribed_teams) {
            append_uuid36(payload, team->uuid);
            append_name32(payload, team->name);
            append_fixed(payload, team->description, kDescWireSize);
        }

        res.response.code = RES_LIST_TEAMS;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // LIST_CHANNELS: list all channels in current team
    if (level == USE_TEAM) {
        team_t *team = find_team_by_uuid(_data, std::string_view(ctx.team_uuid, kUuidWireSize));
        if (!team) {
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        std::vector<uint8_t> payload;
        payload.reserve(4 + team->channels.size() * (kUuidWireSize + kNameWireSize + kDescWireSize));
        
        // Count header
        append_u32(payload, static_cast<uint32_t>(team->channels.size()));

        // Each channel: uuid(36) + name(32) + desc(255)
        for (const auto &channel : team->channels) {
            append_uuid36(payload, channel.uuid);
            append_name32(payload, channel.name);
            append_fixed(payload, channel.description, kDescWireSize);
        }

        res.response.code = RES_LIST_CHANNELS;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // LIST_THREADS: list all threads in current channel
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
        payload.reserve(4 + channel->threads.size() * (kUuidWireSize + kUuidWireSize + kNameWireSize + MAX_BODY_LENGTH + sizeof(uint32_t)));
        
        // Count header
        append_u32(payload, static_cast<uint32_t>(channel->threads.size()));

        // Each thread: uuid(36) + creator_uuid(36) + title(32) + body(512) + timestamp(4)
        for (const auto &thread : channel->threads) {
            append_uuid36(payload, thread.uuid);
            append_uuid36(payload, thread.creator_uuid);
            append_name32(payload, thread.title);
            append_fixed(payload, thread.body, MAX_BODY_LENGTH);
            append_u32(payload, static_cast<uint32_t>(thread.timestamp));
        }

        res.response.code = RES_LIST_THREADS;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // LIST_REPLIES: list all replies in current thread
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
        payload.reserve(4 + thread->replies.size() * (kUuidWireSize + kUuidWireSize + MAX_BODY_LENGTH + sizeof(uint32_t)));
        
        // Count header
        append_u32(payload, static_cast<uint32_t>(thread->replies.size()));

        // Each reply: uuid(36) + creator_uuid(36) + body(512) + timestamp(4)
        for (const auto &reply : thread->replies) {
            append_uuid36(payload, reply.uuid);
            append_uuid36(payload, reply.creator_uuid);
            append_fixed(payload, reply.body, MAX_BODY_LENGTH);
            append_u32(payload, static_cast<uint32_t>(reply.timestamp));
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
