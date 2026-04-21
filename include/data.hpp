/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** data.hpp
*/

#ifndef DATA_HPP
#define DATA_HPP

#include <uuid/uuid.h>
#include <ctime>
#include <string>
#include <vector>
#include <map>

/** @brief Maximum length for names stored in fixed-size protocol fields. */
#define MAX_NAME_LENGTH 32
/** @brief Maximum length for descriptions stored in fixed-size protocol fields. */
#define MAX_DESCRIPTION_LENGTH 255
/** @brief Maximum length for message/thread/reply bodies. */
#define MAX_BODY_LENGTH 512
/** @brief UUID string storage length including trailing null byte. */
#define UUID_STR_LEN 37

/** @brief Reply entity stored inside a thread. */
struct reply_t
{
    /** Reply UUID as a null-terminated string. */
    char uuid[UUID_STR_LEN];
    /** UUID of the user who created the reply. */
    char creator_uuid[UUID_STR_LEN];
    /** Reply content. */
    char body[MAX_BODY_LENGTH];
    /** Creation timestamp. */
    time_t timestamp;
};

/** @brief Discussion thread stored inside a channel. */
struct thread_t
{
    /** Thread UUID as a null-terminated string. */
    char uuid[UUID_STR_LEN];
    /** UUID of the user who created the thread. */
    char creator_uuid[UUID_STR_LEN];
    /** Thread title. */
    char title[MAX_NAME_LENGTH];
    /** Thread body. */
    char body[MAX_BODY_LENGTH];
    /** Creation timestamp. */
    time_t timestamp;
    /** Replies posted in this thread. */
    std::vector<reply_t> replies;
};

/** @brief Team channel containing threads. */
struct channel_t
{
    /** Channel UUID as a null-terminated string. */
    char uuid[UUID_STR_LEN];
    /** Channel display name. */
    char name[MAX_NAME_LENGTH];
    /** Channel description. */
    char description[MAX_DESCRIPTION_LENGTH];
    /** Threads contained in the channel. */
    std::vector<thread_t> threads;
};

/** @brief Team containing channels and subscribed members. */
struct team_t
{
    /** Team UUID as a null-terminated string. */
    char uuid[UUID_STR_LEN];
    /** Team display name. */
    char name[MAX_NAME_LENGTH];
    /** Team description. */
    char description[MAX_DESCRIPTION_LENGTH];
    /** Channels contained in the team. */
    std::vector<channel_t> channels;
    /** UUIDs of subscribed users. */
    std::vector<std::string> member_uuids;
};

/** @brief Direct private message between two users. */
struct message_t
{
    /** UUID of sender user. */
    char sender_uuid[UUID_STR_LEN];
    /** UUID of receiver user. */
    char receiver_uuid[UUID_STR_LEN];
    /** Message content. */
    char body[MAX_BODY_LENGTH];
    /** Message creation timestamp. */
    time_t timestamp;
};

/** @brief User account tracked by the server. */
struct user_t
{
    /** User UUID as a null-terminated string. */
    char uuid[UUID_STR_LEN];
    /** Login/display name. */
    char name[MAX_NAME_LENGTH];
    /** Whether the user is currently authenticated. */
    bool is_logged;
    /** Active socket file descriptor, or stale value if offline. */
    int fd;
};

/** @brief Per-client contextual selection used by /use and contextual commands. */
struct client_context_t
{
    /** Current context level (none/team/channel/thread). */
    uint8_t level;
    /** Selected team UUID (if applicable). */
    char team_uuid[UUID_STR_LEN];
    /** Selected channel UUID (if applicable). */
    char channel_uuid[UUID_STR_LEN];
    /** Selected thread UUID (if applicable). */
    char thread_uuid[UUID_STR_LEN];
};

/** @brief Global mutable server state persisted and shared across handlers. */
struct server_data_t
{
    /** Known users. */
    std::vector<user_t> users;
    /** Active session mapping: socket fd -> authenticated user UUID. */
    std::map<int, std::string> sessions;
    /** Current command context for each connected client. */
    std::map<int, client_context_t> client_contexts;
    /** Teams tree (teams/channels/threads/replies). */
    std::vector<team_t> teams;
    /** Direct private messages. */
    std::vector<message_t> messages;
};

#endif
