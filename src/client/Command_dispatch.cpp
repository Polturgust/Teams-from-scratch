/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Command_dispatch.cpp
*/

#include "Client.hpp"

void Client::_dispatch(const ParsedCommand &cmd)
{
    if (cmd.name == "help")        { _cmd_help(); return; }
    if (cmd.name == "login")       { _cmd_login(cmd); return; }
    if (cmd.name == "logout")      { _cmd_logout(cmd); return; }
    if (cmd.name == "users")       { _cmd_users(cmd); return; }
    if (cmd.name == "user")        { _cmd_user(cmd); return; }
    if (cmd.name == "send")        { _cmd_send(cmd); return; }
    if (cmd.name == "messages")    { _cmd_messages(cmd); return; }
    if (cmd.name == "subscribe")   { _cmd_subscribe(cmd); return; }
    if (cmd.name == "subscribed")  { _cmd_subscribed(cmd); return; }
    if (cmd.name == "unsubscribe") { _cmd_unsubscribe(cmd); return; }
    if (cmd.name == "use")         { _cmd_use(cmd); return; }
    if (cmd.name == "create")      { _cmd_create(cmd); return; }
    if (cmd.name == "list")        { _cmd_list(cmd); return; }
    if (cmd.name == "info")        { _cmd_info(cmd); return; }

    std::cout << "Unknown command: /" << cmd.name << ". Type /help for help." << std::endl;
}
