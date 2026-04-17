/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Individual.cpp
*/

#include "Client.hpp"

void Client::_cmd_login(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /login \"user_name\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 32);
    _send_packet(CMD_LOGIN, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_logout(const ParsedCommand &cmd)
{
    (void)cmd;
    _send_packet(CMD_LOGOUT);
}

void Client::_cmd_users(const ParsedCommand &cmd)
{
    (void)cmd;
    _send_packet(CMD_USERS);
}

void Client::_cmd_user(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /user \"user_uuid\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    _send_packet(CMD_USER, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_send(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 2) {
        std::cerr << "Usage: /send \"user_uuid\" \"message_body\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    pack_fixed(payload, cmd.args[1], 512);
    _send_packet(CMD_SEND, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_messages(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /messages \"user_uuid\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    _send_packet(CMD_MESSAGES, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_subscribe(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /subscribe \"team_uuid\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    _send_packet(CMD_SUBSCRIBE, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_subscribed(const ParsedCommand &cmd)
{
    std::vector<uint8_t> payload;
    if (cmd.args.empty()) {
        payload.push_back(0x00);
    } else if (cmd.args.size() == 1) {
        payload.push_back(0x01);
        pack_fixed(payload, cmd.args[0], 36);
    } else {
        std::cerr << "Usage: /subscribed [\"team_uuid\"]" << std::endl;
        return;
    }
    _send_packet(CMD_SUBSCRIBED, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_unsubscribe(const ParsedCommand &cmd)
{
    if (cmd.args.size() != 1) {
        std::cerr << "Usage: /unsubscribe \"team_uuid\"" << std::endl;
        return;
    }
    std::vector<uint8_t> payload;
    pack_fixed(payload, cmd.args[0], 36);
    _send_packet(CMD_UNSUBSCRIBE, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_use(const ParsedCommand &cmd)
{
    std::vector<uint8_t> payload;

    if (cmd.args.empty()) {
        payload.push_back(USE_NONE);
        _use_level = USE_NONE;
        _use_team_uuid.clear();
        _use_channel_uuid.clear();
        _use_thread_uuid.clear();
    } else if (cmd.args.size() == 1) {
        payload.push_back(USE_TEAM);
        pack_fixed(payload, cmd.args[0], 36);
        _use_level = USE_TEAM;
        _use_team_uuid = cmd.args[0];
        _use_channel_uuid.clear();
        _use_thread_uuid.clear();
    } else if (cmd.args.size() == 2) {
        payload.push_back(USE_CHANNEL);
        pack_fixed(payload, cmd.args[0], 36);
        pack_fixed(payload, cmd.args[1], 36);
        _use_level = USE_CHANNEL;
        _use_team_uuid    = cmd.args[0];
        _use_channel_uuid = cmd.args[1];
        _use_thread_uuid.clear();
    } else if (cmd.args.size() == 3) {
        payload.push_back(USE_THREAD);
        pack_fixed(payload, cmd.args[0], 36);
        pack_fixed(payload, cmd.args[1], 36);
        pack_fixed(payload, cmd.args[2], 36);
        _use_level = USE_THREAD;
        _use_team_uuid    = cmd.args[0];
        _use_channel_uuid = cmd.args[1];
        _use_thread_uuid  = cmd.args[2];
    } else {
        std::cerr << "Usage: /use [\"team_uuid\" [\"channel_uuid\" [\"thread_uuid\"]]]" << std::endl;
        return;
    }
    _send_packet(CMD_USE, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_create(const ParsedCommand &cmd)
{
    std::vector<uint8_t> payload;

    switch (_use_level) {
    case USE_NONE:
        if (cmd.args.size() != 2) {
            std::cerr << "Usage: /create \"team_name\" \"team_description\"" << std::endl;
            return;
        }
        pack_fixed(payload, cmd.args[0], 32);
        pack_fixed(payload, cmd.args[1], 255);
        break;
    case USE_TEAM:
        if (cmd.args.size() != 2) {
            std::cerr << "Usage: /create \"channel_name\" \"channel_description\"" << std::endl;
            return;
        }
        pack_fixed(payload, cmd.args[0], 32);
        pack_fixed(payload, cmd.args[1], 255);
        break;
    case USE_CHANNEL:
        if (cmd.args.size() != 2) {
            std::cerr << "Usage: /create \"thread_title\" \"thread_body\"" << std::endl;
            return;
        }
        pack_fixed(payload, cmd.args[0], 32);
        pack_fixed(payload, cmd.args[1], 512);
        break;
    case USE_THREAD:
        if (cmd.args.size() != 1) {
            std::cerr << "Usage: /create \"comment_body\"" << std::endl;
            return;
        }
        pack_fixed(payload, cmd.args[0], 512);
        break;
    }
    _send_packet(CMD_CREATE, payload.data(), static_cast<uint32_t>(payload.size()));
}

void Client::_cmd_list(const ParsedCommand &cmd)
{
    (void)cmd;
    _send_packet(CMD_LIST);
}

void Client::_cmd_info(const ParsedCommand &cmd)
{
    (void)cmd;
    _send_packet(CMD_INFO);
}
