/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Command_help.cpp
*/

#include "Client.hpp"

void Client::_cmd_help()
{
    std::cout <<
        "Available commands:\n"
        "  /help                                        Show this help\n"
        "  /login \"user_name\"                           Log in to the server\n"
        "  /logout                                      Disconnect from the server\n"
        "  /users                                       List all users\n"
        "  /user \"user_uuid\"                            Get details about a user\n"
        "  /send \"user_uuid\" \"message_body\"             Send a private message\n"
        "  /messages \"user_uuid\"                        List messages with a user\n"
        "  /subscribe \"team_uuid\"                       Subscribe to a team\n"
        "  /subscribed [\"team_uuid\"]                    List subscribed teams or team members\n"
        "  /unsubscribe \"team_uuid\"                     Unsubscribe from a team\n"
        "  /use [\"team_uuid\" [\"channel_uuid\" [\"thread_uuid\"]]]  Set context\n"
        "  /create [args...]                            Create resource (context-dependent)\n"
        "    (no context)          \"team_name\" \"team_description\"\n"
        "    (team)                \"channel_name\" \"channel_description\"\n"
        "    (team+channel)        \"thread_title\" \"thread_body\"\n"
        "    (team+channel+thread) \"comment_body\"\n"
        "  /list                                        List resources (context-dependent)\n"
        "  /info                                        Show info (context-dependent)\n";
}
