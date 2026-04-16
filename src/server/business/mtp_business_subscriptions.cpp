/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_business_subscriptions.cpp
*/

#include "mtp_business.hpp"

#include "mtp_business_detail.hpp"

#include "logging_server.h"
#include "protocole.hpp"

namespace mtp {

using namespace biz_detail;

Result Business::handle_subscribe(int fd, std::string_view team_uuid)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    if (!is_uuid36(team_uuid)) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string user_uuid = it->second;
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
        team->member_uuids.push_back(user_uuid);
        server_event_user_subscribed(team->uuid, user_uuid.c_str());
    }

    std::vector<uint8_t> payload;
    payload.reserve(kUuidWireSize);
    append_fixed(payload, team->uuid, kUuidWireSize);

    res.response.code = RES_SUBSCRIBE_OK;
    res.response.bytes = make_message(res.response.code, payload);
    return res;
}

Result Business::handle_unsubscribe(int fd, std::string_view team_uuid)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    if (!is_uuid36(team_uuid)) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string user_uuid = it->second;
    team_t *team = find_team_by_uuid(_data, team_uuid);

    if (!team) {
        std::vector<uint8_t> payload;
        payload.reserve(kUuidWireSize);
        append_fixed(payload, team_uuid.data(), kUuidWireSize);
        res.response.code = ERR_UNKNOWN_TEAM;
        res.response.bytes = make_message(res.response.code, payload);
        return res;
    }

    auto &members = team->member_uuids;
    const auto mit = std::find(members.begin(), members.end(), user_uuid);
    if (mit == members.end()) {
        res.response.code = ERR_NOT_SUBSCRIBED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    members.erase(mit);
    server_event_user_unsubscribed(team->uuid, user_uuid.c_str());

    std::vector<uint8_t> payload;
    payload.reserve(kUuidWireSize);
    append_fixed(payload, team->uuid, kUuidWireSize);

    res.response.code = RES_UNSUBSCRIBE_OK;
    res.response.bytes = make_message(res.response.code, payload);
    return res;
}

Result Business::handle_subscribed_teams(int fd)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string user_uuid = it->second;

    std::vector<const team_t *> subscribed;
    subscribed.reserve(_data.teams.size());
    for (const auto &t : _data.teams) {
        if (is_subscribed_to(t, user_uuid))
            subscribed.push_back(&t);
    }

    std::vector<uint8_t> payload;
    append_u32(payload, static_cast<uint32_t>(subscribed.size()));

    for (const auto *t : subscribed) {
        append_fixed(payload, t->uuid, kUuidWireSize);
        append_fixed(payload, t->name, kNameWireSize);
        append_fixed(payload, t->description, kDescWireSize);
    }

    res.response.code = RES_SUBSCRIBED_TEAMS;
    res.response.bytes = make_message(res.response.code, payload);
    return res;
}

Result Business::handle_subscribed_users(int fd, std::string_view team_uuid)
{
    Result res;

    const auto it = _data.sessions.find(fd);
    if (it == _data.sessions.end()) {
        res.response.code = ERR_UNAUTHORIZED;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    if (!is_uuid36(team_uuid)) {
        res.response.code = ERR_INVALID_COMMAND;
        res.response.bytes = make_message(res.response.code, {});
        return res;
    }

    const std::string user_uuid = it->second;
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

    std::vector<const user_t *> users;
    users.reserve(team->member_uuids.size());
    for (const auto &member_uuid : team->member_uuids) {
        user_t *u = find_user_by_uuid(_data, member_uuid);
        if (u)
            users.push_back(u);
    }

    std::vector<uint8_t> payload;
    append_u32(payload, static_cast<uint32_t>(users.size()));

    for (const auto *u : users) {
        append_uuid36(payload, u->uuid);
        append_name32(payload, u->name);
        const uint8_t status = is_user_online(_data, *u) ? 0x01 : 0x00;
        append_fixed(payload, &status, sizeof(status));
    }

    res.response.code = RES_SUBSCRIBED_USERS;
    res.response.bytes = make_message(res.response.code, payload);
    return res;
}

} // namespace mtp
