/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Server_response.cpp
*/

#include "Client.hpp"

extern "C" {
    #include "../../libs/myteams/logging_client.h"
}

void Client::_handle_response(uint16_t code, const std::vector<uint8_t> &p)
{
    size_t off = 0;

    switch (code) {
    case RES_LOGIN_OK: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        _user_uuid = uuid;
        client_event_logged_in(uuid.c_str(), name.c_str());
        break;
    }
    case RES_LOGOUT_OK: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        client_event_logged_out(uuid.c_str(), name.c_str());
        _running = false;
        break;
    }
    case RES_USERS_LIST: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid   = read_fixed(p, off, 36);
            std::string name   = read_fixed(p, off, 32);
            uint8_t     status = read_u8(p, off);
            client_print_users(uuid.c_str(), name.c_str(), static_cast<int>(status));
        }
        break;
    }
    case RES_USER_INFO: {
        std::string uuid   = read_fixed(p, off, 36);
        std::string name   = read_fixed(p, off, 32);
        uint8_t     status = read_u8(p, off);
        client_print_user(uuid.c_str(), name.c_str(), static_cast<int>(status));
        break;
    }
    case RES_SEND_OK:
        break;
    case RES_MESSAGES_LIST: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string sender   = read_fixed(p, off, 36);
            /* receiver_uuid */    read_fixed(p, off, 36); // consumed but not needed for display
            int64_t     ts       = read_i64(p, off);
            std::string body     = read_fixed(p, off, 512);
            client_private_message_print_messages(sender.c_str(),
                static_cast<time_t>(ts), body.c_str());
        }
        break;
    }
    case RES_SUBSCRIBE_OK: {
        std::string team_uuid = read_fixed(p, off, 36);
        client_print_subscribed(_user_uuid.c_str(), team_uuid.c_str());
        break;
    }
    case RES_SUBSCRIBED_TEAMS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid = read_fixed(p, off, 36);
            std::string name = read_fixed(p, off, 32);
            std::string desc = read_fixed(p, off, 255);
            client_print_teams(uuid.c_str(), name.c_str(), desc.c_str());
        }
        break;
    }
    case RES_SUBSCRIBED_USERS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid   = read_fixed(p, off, 36);
            std::string name   = read_fixed(p, off, 32);
            uint8_t     status = read_u8(p, off);
            client_print_users(uuid.c_str(), name.c_str(), static_cast<int>(status));
        }
        break;
    }
    case RES_UNSUBSCRIBE_OK: {
        std::string team_uuid = read_fixed(p, off, 36);
        client_print_unsubscribed(_user_uuid.c_str(), team_uuid.c_str());
        break;
    }
    case RES_TEAM_CREATED: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_print_team_created(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case RES_CHANNEL_CREATED: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_print_channel_created(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case RES_THREAD_CREATED: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        int64_t     ts           = read_i64(p, off);
        std::string title        = read_fixed(p, off, 32);
        std::string body         = read_fixed(p, off, 512);
        client_print_thread_created(thread_uuid.c_str(), creator_uuid.c_str(),
            static_cast<time_t>(ts), title.c_str(), body.c_str());
        break;
    }
    case RES_REPLY_CREATED: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        int64_t     ts           = read_i64(p, off);
        std::string body         = read_fixed(p, off, 512);
        client_print_reply_created(thread_uuid.c_str(), creator_uuid.c_str(),
            static_cast<time_t>(ts), body.c_str());
        break;
    }
    case RES_LIST_TEAMS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid = read_fixed(p, off, 36);
            std::string name = read_fixed(p, off, 32);
            std::string desc = read_fixed(p, off, 255);
            client_print_teams(uuid.c_str(), name.c_str(), desc.c_str());
        }
        break;
    }
    case RES_LIST_CHANNELS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string uuid = read_fixed(p, off, 36);
            std::string name = read_fixed(p, off, 32);
            std::string desc = read_fixed(p, off, 255);
            client_team_print_channels(uuid.c_str(), name.c_str(), desc.c_str());
        }
        break;
    }
    case RES_LIST_THREADS: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string thread_uuid  = read_fixed(p, off, 36);
            std::string creator_uuid = read_fixed(p, off, 36);
            int64_t     ts           = read_i64(p, off);
            std::string title        = read_fixed(p, off, 32);
            std::string body         = read_fixed(p, off, 512);
            client_channel_print_threads(thread_uuid.c_str(), creator_uuid.c_str(),
                static_cast<time_t>(ts), title.c_str(), body.c_str());
        }
        break;
    }
    case RES_LIST_REPLIES: {
        uint32_t count = read_u32(p, off);
        for (uint32_t i = 0; i < count; ++i) {
            std::string thread_uuid  = read_fixed(p, off, 36);
            std::string creator_uuid = read_fixed(p, off, 36);
            int64_t     ts           = read_i64(p, off);
            std::string body         = read_fixed(p, off, 512);
            client_thread_print_replies(thread_uuid.c_str(), creator_uuid.c_str(),
                static_cast<time_t>(ts), body.c_str());
        }
        break;
    }
    case RES_TEAM_INFO: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_print_team(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case RES_CHANNEL_INFO: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_print_channel(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case RES_THREAD_INFO: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        int64_t     ts           = read_i64(p, off);
        std::string title        = read_fixed(p, off, 32);
        std::string body         = read_fixed(p, off, 512);
        client_print_thread(thread_uuid.c_str(), creator_uuid.c_str(),
            static_cast<time_t>(ts), title.c_str(), body.c_str());
        break;
    }
    case RES_USER_DETAILS: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        client_print_user(uuid.c_str(), name.c_str(), -1);
        break;
    }

    case ERR_UNAUTHORIZED:
        client_error_unauthorized();
        break;
    case ERR_UNKNOWN_TEAM: {
        std::string uuid = read_fixed(p, off, 36);
        client_error_unknown_team(uuid.c_str());
        break;
    }
    case ERR_UNKNOWN_CHANNEL: {
        std::string uuid = read_fixed(p, off, 36);
        client_error_unknown_channel(uuid.c_str());
        break;
    }
    case ERR_UNKNOWN_THREAD: {
        std::string uuid = read_fixed(p, off, 36);
        client_error_unknown_thread(uuid.c_str());
        break;
    }
    case ERR_UNKNOWN_USER: {
        std::string uuid = read_fixed(p, off, 36);
        client_error_unknown_user(uuid.c_str());
        break;
    }
    case ERR_ALREADY_EXISTS:
        client_error_already_exist();
        break;
    case ERR_NOT_SUBSCRIBED:
        client_error_unauthorized();
        break;
    case ERR_INVALID_COMMAND:
        std::cerr << "Server: invalid command." << std::endl;
        break;

    case EVT_MESSAGE_RECEIVED: {
        std::string sender = read_fixed(p, off, 36);
        std::string body   = read_fixed(p, off, 512);
        client_event_private_message_received(sender.c_str(), body.c_str());
        break;
    }
    case EVT_TEAM_CREATED: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_event_team_created(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case EVT_CHANNEL_CREATED: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        std::string desc = read_fixed(p, off, 255);
        client_event_channel_created(uuid.c_str(), name.c_str(), desc.c_str());
        break;
    }
    case EVT_THREAD_CREATED: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        int64_t     ts           = read_i64(p, off);
        std::string title        = read_fixed(p, off, 32);
        std::string body         = read_fixed(p, off, 512);
        client_event_thread_created(thread_uuid.c_str(), creator_uuid.c_str(),
            static_cast<time_t>(ts), title.c_str(), body.c_str());
        break;
    }
    case EVT_REPLY_CREATED: {
        std::string thread_uuid  = read_fixed(p, off, 36);
        std::string creator_uuid = read_fixed(p, off, 36);
        /* timestamp */            read_i64(p, off);
        std::string body         = read_fixed(p, off, 512);
        client_event_thread_reply_received(
            _use_team_uuid.empty() ? "" : _use_team_uuid.c_str(),
            thread_uuid.c_str(), creator_uuid.c_str(), body.c_str());
        break;
    }
    case EVT_USER_LOGGED_IN: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        client_event_logged_in(uuid.c_str(), name.c_str());
        break;
    }
    case EVT_USER_LOGGED_OUT: {
        std::string uuid = read_fixed(p, off, 36);
        std::string name = read_fixed(p, off, 32);
        client_event_logged_out(uuid.c_str(), name.c_str());
        break;
    }

    default:
        std::cerr << "Unknown response code: 0x" << std::hex << code << std::dec << std::endl;
        break;
    }
}

