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

    if (code == RES_LOGIN_OK) {
        auto uuid = read_fixed(p, off, 36), name = read_fixed(p, off, 32);
        _user_uuid = uuid;
        client_event_logged_in(uuid.c_str(), name.c_str()); return;
    }
    if (code == RES_LOGOUT_OK) {
        auto uuid = read_fixed(p, off, 36), name = read_fixed(p, off, 32);
        client_event_logged_out(uuid.c_str(), name.c_str());
        _running = false; return;
    }
    if (code == RES_SEND_OK)       return;
    if (code == RES_SUBSCRIBE_OK)  { client_print_subscribed(_user_uuid.c_str(),   read_fixed(p, off, 36).c_str()); return; }
    if (code == RES_UNSUBSCRIBE_OK){ client_print_unsubscribed(_user_uuid.c_str(), read_fixed(p, off, 36).c_str()); return; }
    if (code == RES_USER_INFO || code == RES_USER_DETAILS) {
        auto uuid = read_fixed(p, off, 36), name = read_fixed(p, off, 32);
        int  status = (code == RES_USER_INFO) ? (int)read_u8(p, off) : -1;
        client_print_user(uuid.c_str(), name.c_str(), status); return;
    }
    if (code == RES_TEAM_INFO || code == RES_TEAM_CREATED) {
        auto uuid = read_fixed(p, off, 36), name = read_fixed(p, off, 32), desc = read_fixed(p, off, 255);
        if (code == RES_TEAM_CREATED) client_print_team_created(uuid.c_str(), name.c_str(), desc.c_str());
        else                          client_print_team(uuid.c_str(), name.c_str(), desc.c_str());
        return;
    }
    if (code == RES_CHANNEL_INFO || code == RES_CHANNEL_CREATED) {
        auto uuid = read_fixed(p, off, 36), name = read_fixed(p, off, 32), desc = read_fixed(p, off, 255);
        if (code == RES_CHANNEL_CREATED) client_print_channel_created(uuid.c_str(), name.c_str(), desc.c_str());
        else                             client_print_channel(uuid.c_str(), name.c_str(), desc.c_str());
        return;
    }
    if (code == RES_THREAD_INFO || code == RES_THREAD_CREATED) {
        auto tuuid = read_fixed(p, off, 36), cuuid = read_fixed(p, off, 36);
        auto ts = read_i64(p, off);
        auto title = read_fixed(p, off, 32), body = read_fixed(p, off, 512);
        if (code == RES_THREAD_CREATED) client_print_thread_created(tuuid.c_str(), cuuid.c_str(), ts, title.c_str(), body.c_str());
        else                            client_print_thread(tuuid.c_str(), cuuid.c_str(), ts, title.c_str(), body.c_str());
        return;
    }
    if (code == RES_REPLY_CREATED) {
        auto tuuid = read_fixed(p, off, 36), cuuid = read_fixed(p, off, 36);
        auto ts = read_i64(p, off);
        client_print_reply_created(tuuid.c_str(), cuuid.c_str(), ts, read_fixed(p, off, 512).c_str()); return;
    }
    auto list_users = [&](auto fn) {
        for (uint32_t i = 0, n = read_u32(p, off); i < n; ++i) {
            auto uuid = read_fixed(p, off, 36), name = read_fixed(p, off, 32);
            fn(uuid.c_str(), name.c_str(), (int)read_u8(p, off));
        }
    };
    auto list_teams = [&](auto fn) {
        for (uint32_t i = 0, n = read_u32(p, off); i < n; ++i) {
            auto uuid = read_fixed(p, off, 36), name = read_fixed(p, off, 32), desc = read_fixed(p, off, 255);
            fn(uuid.c_str(), name.c_str(), desc.c_str());
        }
    };
    if (code == RES_USERS_LIST || code == RES_SUBSCRIBED_USERS) { list_users(client_print_users);        return; }
    if (code == RES_LIST_TEAMS || code == RES_SUBSCRIBED_TEAMS)  { list_teams(client_print_teams);        return; }
    if (code == RES_LIST_CHANNELS)                               { list_teams(client_team_print_channels); return; }
    if (code == RES_LIST_THREADS) {
        for (uint32_t i = 0, n = read_u32(p, off); i < n; ++i) {
            auto tuuid = read_fixed(p, off, 36), cuuid = read_fixed(p, off, 36);
            auto ts = read_i64(p, off);
            auto title = read_fixed(p, off, 32), body = read_fixed(p, off, 512);
            client_channel_print_threads(tuuid.c_str(), cuuid.c_str(), ts, title.c_str(), body.c_str());
        }
        return;
    }
    if (code == RES_LIST_REPLIES) {
        for (uint32_t i = 0, n = read_u32(p, off); i < n; ++i) {
            auto tuuid = read_fixed(p, off, 36), cuuid = read_fixed(p, off, 36);
            auto ts = read_i64(p, off);
            client_thread_print_replies(tuuid.c_str(), cuuid.c_str(), ts, read_fixed(p, off, 512).c_str());
        }
        return;
    }
    if (code == RES_MESSAGES_LIST) {
        for (uint32_t i = 0, n = read_u32(p, off); i < n; ++i) {
            auto sender = read_fixed(p, off, 36); read_fixed(p, off, 36);
            auto ts = read_i64(p, off);
            client_private_message_print_messages(sender.c_str(), ts, read_fixed(p, off, 512).c_str());
        }
        return;
    }
    if (code == ERR_UNAUTHORIZED || code == ERR_NOT_SUBSCRIBED) { client_error_unauthorized();                                   return; }
    if (code == ERR_ALREADY_EXISTS)  { client_error_already_exist();                                                             return; }
    if (code == ERR_UNKNOWN_TEAM)    { client_error_unknown_team(read_fixed(p, off, 36).c_str());                                return; }
    if (code == ERR_UNKNOWN_CHANNEL) { client_error_unknown_channel(read_fixed(p, off, 36).c_str());                            return; }
    if (code == ERR_UNKNOWN_THREAD)  { client_error_unknown_thread(read_fixed(p, off, 36).c_str());                             return; }
    if (code == ERR_UNKNOWN_USER)    { client_error_unknown_user(read_fixed(p, off, 36).c_str());                               return; }
    if (code == ERR_INVALID_COMMAND) { std::cout << "invalid command\n";                                                        return; }
    if (code == EVT_USER_LOGGED_IN)  { auto u = read_fixed(p,off,36), n = read_fixed(p,off,32); client_event_logged_in(u.c_str(), n.c_str());  return; }
    if (code == EVT_USER_LOGGED_OUT) { auto u = read_fixed(p,off,36), n = read_fixed(p,off,32); client_event_logged_out(u.c_str(), n.c_str()); return; }
    if (code == EVT_MESSAGE_RECEIVED){
        auto sender = read_fixed(p, off, 36);
        client_event_private_message_received(sender.c_str(), read_fixed(p, off, 512).c_str()); return;
    }
    if (code == EVT_TEAM_CREATED) {
        auto uuid = read_fixed(p,off,36), name = read_fixed(p,off,32), desc = read_fixed(p,off,255);
        client_event_team_created(uuid.c_str(), name.c_str(), desc.c_str()); return;
    }
    if (code == EVT_CHANNEL_CREATED) {
        auto uuid = read_fixed(p,off,36), name = read_fixed(p,off,32), desc = read_fixed(p,off,255);
        client_event_channel_created(uuid.c_str(), name.c_str(), desc.c_str()); return;
    }
    if (code == EVT_THREAD_CREATED) {
        auto tuuid = read_fixed(p,off,36), cuuid = read_fixed(p,off,36);
        auto ts = read_i64(p, off);
        auto title = read_fixed(p,off,32), body = read_fixed(p,off,512);
        client_event_thread_created(tuuid.c_str(), cuuid.c_str(), ts, title.c_str(), body.c_str()); return;
    }
    if (code == EVT_REPLY_CREATED) {
        auto tuuid = read_fixed(p,off,36), cuuid = read_fixed(p,off,36);
        read_i64(p, off);
        client_event_thread_reply_received(
            _use_team_uuid.c_str(), tuuid.c_str(), cuuid.c_str(), read_fixed(p,off,512).c_str());
        return;
    }
}
