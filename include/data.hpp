/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** data.hpp
*/

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

