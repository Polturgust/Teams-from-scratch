/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** mtp.hpp
*/

#ifndef MTP_HPP
#define MTP_HPP

#include <cstdint>
#include <string_view>
#include <vector>

#include "data.hpp"

namespace mtp {

/** @brief Generic protocol packet containing code and payload bytes. */
struct Packet {
    /** Command/response/event/error code. */
    uint16_t code;
    /** Raw payload bytes. */
    std::vector<uint8_t> bytes;
};

/** @brief Outbound event packet targeting one or more client file descriptors. */
struct Push {
    /** Event packet to send. */
    Packet packet;
    /** Destination socket file descriptors. */
    std::vector<int> fds;
};

/** @brief Handler result: one direct response and optional asynchronous pushes. */
struct Result {
    /** Response packet returned to command issuer. */
    Packet response;
    /** Additional events to push to recipients. */
    std::vector<Push> pushes;
};

/** @brief Business layer implementing all MTP command handlers. */
class Business {
public:
    /** @brief Build business handlers over shared mutable server data. */
    explicit Business(server_data_t &data);

    /** @brief Authenticate a user by name or create one if needed. */
    Result handle_login(int fd, std::string_view name);
    /** @brief Logout currently authenticated user. */
    Result handle_logout(int fd);

    /** @brief List all known users with status information. */
    Result handle_users(int fd);
    /** @brief Return details for one user UUID. */
    Result handle_user(int fd, std::string_view user_uuid);

    /** @brief Send a private message to another user UUID. */
    Result handle_send(int fd, std::string_view receiver_uuid, std::string_view body);
    /** @brief Fetch private conversation history with another user. */
    Result handle_messages(int fd, std::string_view user_uuid);

    /** @brief Subscribe authenticated user to a team UUID. */
    Result handle_subscribe(int fd, std::string_view team_uuid);
    /** @brief Unsubscribe authenticated user from a team UUID. */
    Result handle_unsubscribe(int fd, std::string_view team_uuid);
    /** @brief List teams subscribed by authenticated user. */
    Result handle_subscribed_teams(int fd);
    /** @brief List users subscribed to a given team UUID. */
    Result handle_subscribed_users(int fd, std::string_view team_uuid);

    /** @brief Update current /use context from serialized payload bytes. */
    Result handle_use(int fd, std::string_view payload_bytes);
    /** @brief Create resource according to active context and payload bytes. */
    Result handle_create(int fd, std::string_view payload_bytes);
    /** @brief List resources according to active context. */
    Result handle_list(int fd);
    /** @brief Retrieve information for current active context. */
    Result handle_info(int fd);

private:
    /** Shared mutable server state backing all handlers. */
    server_data_t &_data;
};

} // namespace mtp

#endif
