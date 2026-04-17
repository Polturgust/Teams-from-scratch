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

#include "logging_server.h"
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

        team.member_uuids.push_back(user_uuid);

        _data.teams.push_back(team);

        server_event_team_created(team.uuid, team.name, user_uuid.c_str());

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kNameWireSize + kDescWireSize);
        append_uuid36(payload, team.uuid);
        append_name32(payload, team.name);
        append_fixed(payload, team.description, kDescWireSize);

        res.response.code = RES_TEAM_CREATED;
        res.response.bytes = make_message(res.response.code, payload);

        Push evt;
        evt.packet.code = EVT_TEAM_CREATED;
        evt.packet.bytes = make_message(evt.packet.code, payload);
        evt.fds = logged_fds(_data);
        res.pushes.push_back(std::move(evt));
        return res;
    }

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

        if (!is_subscribed_to(*team, user_uuid)) {
            res.response.code = ERR_NOT_SUBSCRIBED;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        channel_t channel{};
        new_uuid(channel.uuid);
        std::memcpy(channel.name, payload_bytes.data(), kNameWireSize);
        std::memcpy(channel.description, payload_bytes.data() + kNameWireSize, kDescWireSize);

        team->channels.push_back(channel);

        server_event_channel_created(team->uuid, channel.uuid, channel.name);

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kNameWireSize + kDescWireSize);
        append_uuid36(payload, channel.uuid);
        append_name32(payload, channel.name);
        append_fixed(payload, channel.description, kDescWireSize);

        res.response.code = RES_CHANNEL_CREATED;
        res.response.bytes = make_message(res.response.code, payload);

        Push evt;
        evt.packet.code = EVT_CHANNEL_CREATED;
        evt.packet.bytes = make_message(evt.packet.code, payload);
        evt.fds = team_member_fds(_data, *team);
        res.pushes.push_back(std::move(evt));
        return res;
    }

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

        if (!is_subscribed_to(*team, user_uuid)) {
            res.response.code = ERR_NOT_SUBSCRIBED;
            res.response.bytes = make_message(res.response.code, {});
            return res;
        }

        channel_t *channel = find_channel_by_uuid(*team, std::string_view(ctx.channel_uuid, kUuidWireSize));

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

        server_event_thread_created(channel->uuid, thread.uuid, user_uuid.c_str(), thread.title, thread.body);

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kUuidWireSize + sizeof(uint64_t) + kNameWireSize + MAX_BODY_LENGTH);
        append_uuid36(payload, thread.uuid);
        append_uuid36(payload, thread.creator_uuid);
        append_i64(payload, static_cast<int64_t>(thread.timestamp));
        append_name32(payload, thread.title);
        append_fixed(payload, thread.body, MAX_BODY_LENGTH);

        res.response.code = RES_THREAD_CREATED;
        res.response.bytes = make_message(res.response.code, payload);

        Push evt;
        evt.packet.code = EVT_THREAD_CREATED;
        evt.packet.bytes = make_message(evt.packet.code, payload);
        evt.fds = team_member_fds(_data, *team);
        res.pushes.push_back(std::move(evt));
        return res;
    }

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

        if (!is_subscribed_to(*team, user_uuid)) {
            res.response.code = ERR_NOT_SUBSCRIBED;
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

        reply_t reply{};
        new_uuid(reply.uuid);
        std::memcpy(reply.creator_uuid, user_uuid.data(), kUuidWireSize);
        std::memcpy(reply.body, payload_bytes.data(), MAX_BODY_LENGTH);
        reply.timestamp = std::time(nullptr);

        thread->replies.push_back(reply);

        server_event_reply_created(thread->uuid, user_uuid.c_str(), reply.body);

        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kUuidWireSize + sizeof(uint64_t) + MAX_BODY_LENGTH);
        append_fixed(payload, ctx.thread_uuid, kUuidWireSize);
        append_uuid36(payload, reply.creator_uuid);
        append_i64(payload, static_cast<int64_t>(reply.timestamp));
        append_fixed(payload, reply.body, MAX_BODY_LENGTH);

        res.response.code = RES_REPLY_CREATED;
        res.response.bytes = make_message(res.response.code, payload);

        Push evt;
        evt.packet.code = EVT_REPLY_CREATED;
        evt.packet.bytes = make_message(evt.packet.code, payload);
        evt.fds = team_member_fds(_data, *team);
        res.pushes.push_back(std::move(evt));
        return res;
    }

    res.response.code = ERR_INVALID_COMMAND;
    res.response.bytes = make_message(res.response.code, {});
    return res;
}

} // namespace mtp
