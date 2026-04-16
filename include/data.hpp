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

#define MAX_NAME_LENGTH 32
#define MAX_DESCRIPTION_LENGTH 255
#define MAX_BODY_LENGTH 512
#define UUID_STR_LEN 37

struct reply_t
{
    char uuid[UUID_STR_LEN];
    char creator_uuid[UUID_STR_LEN];
    char body[MAX_BODY_LENGTH];
    time_t timestamp;
};

struct thread_t
{
    char uuid[UUID_STR_LEN];
    char creator_uuid[UUID_STR_LEN];
    char title[MAX_NAME_LENGTH];
    char body[MAX_BODY_LENGTH];
    time_t timestamp;
    std::vector<reply_t> replies;
};

struct channel_t
{
    char uuid[UUID_STR_LEN];
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESCRIPTION_LENGTH];
    std::vector<thread_t> threads;
};

struct team_t
{
    char uuid[UUID_STR_LEN];
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESCRIPTION_LENGTH];
    std::vector<channel_t> channels;
    std::vector<std::string> member_uuids;
};

struct message_t
{
    char sender_uuid[UUID_STR_LEN];
    char receiver_uuid[UUID_STR_LEN];
    char body[MAX_BODY_LENGTH];
    time_t timestamp;
};

struct user_t
{
    char uuid[UUID_STR_LEN];
    char name[MAX_NAME_LENGTH];
    bool is_logged;
    int fd;
};

struct server_data_t
{
    std::vector<user_t> users;
    // Logged-in sessions indexed by client fd.
    // Allows multiple simultaneous connections for the same user.
    std::map<int, std::string> sessions;
    std::vector<team_t> teams;
    std::vector<message_t> messages;
};

#endif
