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

    // Initialize or reset context
    client_context_t ctx{};
    std::memset(&ctx, 0, sizeof(ctx));
    ctx.level = level;

    if (level == USE_NONE) {
        // Reset context: respond with RES_USER_DETAILS
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

        std::memcpy(ctx.team_uuid, team_uuid.data(), kUuidWireSize);
        _data.client_contexts[fd] = ctx;

        // RES_TEAM_INFO: uuid(36) + name(32) + desc(255)
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

        channel_t *channel = nullptr;
        for (auto &c : team->channels) {
            if (std::string_view(c.uuid, kUuidWireSize) == channel_uuid) {
                channel = &c;
                break;
            }
        }

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

        // RES_CHANNEL_INFO: uuid(36) + name(32) + desc(255)
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

        channel_t *channel = nullptr;
        for (auto &c : team->channels) {
            if (std::string_view(c.uuid, kUuidWireSize) == channel_uuid) {
                channel = &c;
                break;
            }
        }

        if (!channel) {
            std::vector<uint8_t> payload;
            payload.reserve(kUuidWireSize);
            append_fixed(payload, channel_uuid.data(), kUuidWireSize);
            res.response.code = ERR_UNKNOWN_CHANNEL;
            res.response.bytes = make_message(res.response.code, payload);
            return res;
        }

        thread_t *thread = nullptr;
        for (auto &t : channel->threads) {
            if (std::string_view(t.uuid, kUuidWireSize) == thread_uuid) {
                thread = &t;
                break;
            }
        }

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

        // RES_THREAD_INFO: uuid(36) + creator_uuid(36) + timestamp(8) + title(32) + body(512)
        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize + kUuidWireSize + 8 + kNameWireSize + MAX_BODY_LENGTH);
        append_uuid36(payload, thread->uuid);
        append_uuid36(payload, thread->creator_uuid);

        auto htonll = [](uint64_t v) -> uint64_t {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return (static_cast<uint64_t>(htonl(static_cast<uint32_t>(v & 0xFFFFFFFFULL))) << 32) |
                htonl(static_cast<uint32_t>(v >> 32));
#else
            return v;
#endif
        };

        const int64_t ts = static_cast<int64_t>(thread->timestamp);
        const uint64_t ts_be = htonll(static_cast<uint64_t>(ts));
        append_fixed(payload, &ts_be, sizeof(ts_be));

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
