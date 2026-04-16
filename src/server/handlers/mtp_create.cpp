/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_create.cpp
*/

#include "mtp.hpp"

#include <cstring>
#include <ctime>

#include "mtp_detail.hpp"

#include "protocole.hpp"

namespace mtp {

using namespace detail;

Result Business::handle_create(int fd, std::string_view payload_bytes)
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

    // CREATE_TEAM: payload = name(32) + description(255)
    if (level == USE_NONE) {
        if (payload_bytes.size() < kNameWireSize + kDescWireSize) {
            res.response.code = ERR_INVALID_COMMAND;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        team_t team{};
        new_uuid(team.uuid);
        std::memcpy(team.name, payload_bytes.data(), kNameWireSize);
        std::memcpy(team.description, payload_bytes.data() + kNameWireSize, kDescWireSize);
        
        // Auto-subscribe creator to the team
        team.member_uuids.push_back(user_uuid);
        
        _data.teams.push_back(team);

        // Response: uuid(36) + name(32) + description(255) 
        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kNameWireSize + kDescWireSize);
        append_uuid36(payload, team.uuid);
        append_name32(payload, team.name);
        append_fixed(payload, team.description, kDescWireSize);

        res.response.code = RES_TEAM_CREATED;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // CREATE_CHANNEL: payload = name(32) + description(255)
    if (level == USE_TEAM) {
        if (payload_bytes.size() < kNameWireSize + kDescWireSize) {
            res.response.code = ERR_INVALID_COMMAND;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        team_t *team = find_team_by_uuid(_data, std::string_view(ctx.team_uuid, kUuidWireSize));
        if (!team) {
            res.response.code = ERR_UNKNOWN_TEAM;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        channel_t channel{};
        new_uuid(channel.uuid);
        std::memcpy(channel.name, payload_bytes.data(), kNameWireSize);
        std::memcpy(channel.description, payload_bytes.data() + kNameWireSize, kDescWireSize);
        
        team->channels.push_back(channel);

        // Response: uuid(36) + name(32) + description(255)
        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kNameWireSize + kDescWireSize);
        append_uuid36(payload, channel.uuid);
        append_name32(payload, channel.name);
        append_fixed(payload, channel.description, kDescWireSize);

        res.response.code = RES_CHANNEL_CREATED;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // CREATE_THREAD: payload = title(32) + body(512)
    if (level == USE_CHANNEL) {
        if (payload_bytes.size() < kNameWireSize + MAX_BODY_LENGTH) {
            res.response.code = ERR_INVALID_COMMAND;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

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

        thread_t thread{};
        new_uuid(thread.uuid);
        std::memcpy(thread.creator_uuid, user_uuid.data(), kUuidWireSize);
        std::memcpy(thread.title, payload_bytes.data(), kNameWireSize);
        std::memcpy(thread.body, payload_bytes.data() + kNameWireSize, MAX_BODY_LENGTH);
        thread.timestamp = std::time(nullptr);

        channel->threads.push_back(thread);

        // Response: uuid(36) + creator_uuid(36) + title(32) + body(512) + timestamp(4)
        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kUuidWireSize + kNameWireSize + MAX_BODY_LENGTH + sizeof(uint32_t));
        append_uuid36(payload, thread.uuid);
        append_uuid36(payload, thread.creator_uuid);
        append_name32(payload, thread.title);
        append_fixed(payload, thread.body, MAX_BODY_LENGTH);
        append_u32(payload, static_cast<uint32_t>(thread.timestamp));

        res.response.code = RES_THREAD_CREATED;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    // CREATE_REPLY: payload = body(512)
    if (level == USE_THREAD) {
        if (payload_bytes.size() < MAX_BODY_LENGTH) {
            res.response.code = ERR_INVALID_COMMAND;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

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

        reply_t reply{};
        new_uuid(reply.uuid);
        std::memcpy(reply.creator_uuid, user_uuid.data(), kUuidWireSize);
        std::memcpy(reply.body, payload_bytes.data(), MAX_BODY_LENGTH);
        reply.timestamp = std::time(nullptr);

        thread->replies.push_back(reply);

        // Response: uuid(36) + creator_uuid(36) + body(512) + timestamp(4)
        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kUuidWireSize + MAX_BODY_LENGTH + sizeof(uint32_t));
        append_uuid36(payload, reply.uuid);
        append_uuid36(payload, reply.creator_uuid);
        append_fixed(payload, reply.body, MAX_BODY_LENGTH);
        append_u32(payload, static_cast<uint32_t>(reply.timestamp));

        res.response.code = RES_REPLY_CREATED;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    res.response.code = ERR_INVALID_COMMAND;
    res.response.bytes = make_message(res.response.code, {});
    return res;
}

} // namespace mtp
