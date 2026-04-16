/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp_detail.hpp
*/

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <arpa/inet.h>
#include <uuid/uuid.h>

#include "data.hpp"

namespace mtp::detail {

inline constexpr std::size_t kHeaderSize = 6;
inline constexpr std::size_t kUuidWireSize = 36;
inline constexpr std::size_t kNameWireSize = MAX_NAME_LENGTH;
inline constexpr std::size_t kDescWireSize = MAX_DESCRIPTION_LENGTH;

inline std::vector<uint8_t> make_message(uint16_t code, const std::vector<uint8_t> &payload)
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

inline void append_fixed(std::vector<uint8_t> &out, const void *data, std::size_t size)
{
    const uint8_t *ptr = static_cast<const uint8_t *>(data);
    out.insert(out.end(), ptr, ptr + size);
}

inline void append_u32(std::vector<uint8_t> &out, uint32_t value)
{
    const uint32_t be = htonl(value);
    append_fixed(out, &be, sizeof(be));
}

inline uint64_t htonll(uint64_t value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (static_cast<uint64_t>(htonl(static_cast<uint32_t>(value & 0xFFFFFFFFULL))) << 32) |
        htonl(static_cast<uint32_t>(value >> 32));
#else
    return value;
#endif
}

inline void append_i64(std::vector<uint8_t> &out, int64_t value)
{
    const uint64_t be = htonll(static_cast<uint64_t>(value));
    append_fixed(out, &be, sizeof(be));
}

inline void append_uuid36(std::vector<uint8_t> &out, const char *uuid37)
{
    append_fixed(out, uuid37, kUuidWireSize);
}

inline void append_name32(std::vector<uint8_t> &out, const char name32[kNameWireSize])
{
    append_fixed(out, name32, kNameWireSize);
}

inline std::string_view trim_padded_name(std::string_view name)
{
    const auto null_pos = name.find('\0');
    if (null_pos != std::string_view::npos)
        return name.substr(0, null_pos);
    return name;
}

inline user_t *find_user_by_name(server_data_t &data, std::string_view name)
{
    for (auto &user : data.users) {
        const std::string_view uname(user.name, MAX_NAME_LENGTH);
        if (trim_padded_name(uname) == name)
            return &user;
    }
    return nullptr;
}

inline void set_fixed_name(char out_name32[MAX_NAME_LENGTH], std::string_view name)
{
    std::memset(out_name32, 0, MAX_NAME_LENGTH);
    const std::size_t copy_len = std::min<std::size_t>(name.size(), MAX_NAME_LENGTH);
    std::memcpy(out_name32, name.data(), copy_len);
}

inline void new_uuid(char out_uuid37[UUID_STR_LEN])
{
    uuid_t bin;
    uuid_generate(bin);
    uuid_unparse(bin, out_uuid37);
}

inline bool is_logged_in(const server_data_t &data, int fd)
{
    return data.sessions.find(fd) != data.sessions.end();
}

inline bool is_user_online(const server_data_t &data, const user_t &user)
{
    const std::string uuid(user.uuid);
    for (const auto &kv : data.sessions) {
        if (kv.second == uuid)
            return true;
    }
    return false;
}

inline bool is_uuid36(std::string_view uuid)
{
    return uuid.size() == kUuidWireSize;
}

inline user_t *find_user_by_uuid(server_data_t &data, std::string_view uuid36)
{
    if (!is_uuid36(uuid36))
        return nullptr;
    for (auto &u : data.users) {
        if (std::string_view(u.uuid, kUuidWireSize) == uuid36)
            return &u;
    }
    return nullptr;
}

inline team_t *find_team_by_uuid(server_data_t &data, std::string_view uuid36)
{
    if (!is_uuid36(uuid36))
        return nullptr;
    for (auto &t : data.teams) {
        if (std::string_view(t.uuid, kUuidWireSize) == uuid36)
            return &t;
    }
    return nullptr;
}

inline bool is_subscribed_to(const team_t &team, const std::string &user_uuid)
{
    return std::find(team.member_uuids.begin(), team.member_uuids.end(), user_uuid) != team.member_uuids.end();
}

inline channel_t *find_channel_by_uuid(team_t &team, std::string_view uuid36)
{
    if (!is_uuid36(uuid36))
        return nullptr;
    for (auto &channel : team.channels) {
        if (std::string_view(channel.uuid, kUuidWireSize) == uuid36)
            return &channel;
    }
    return nullptr;
}

inline thread_t *find_thread_by_uuid(channel_t &channel, std::string_view uuid36)
{
    if (!is_uuid36(uuid36))
        return nullptr;
    for (auto &thread : channel.threads) {
        if (std::string_view(thread.uuid, kUuidWireSize) == uuid36)
            return &thread;
    }
    return nullptr;
}

inline std::vector<int> logged_fds(const server_data_t &data)
{
    std::vector<int> fds;
    fds.reserve(data.sessions.size());
    for (const auto &kv : data.sessions)
        fds.push_back(kv.first);
    return fds;
}

inline std::vector<int> team_member_fds(const server_data_t &data, const team_t &team)
{
    std::vector<int> fds;
    for (const auto &kv : data.sessions) {
        if (is_subscribed_to(team, kv.second))
            fds.push_back(kv.first);
    }
    return fds;
}

} // namespace mtp::detail
