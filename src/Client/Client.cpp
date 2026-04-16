/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Client.cpp
*/

#include "Client.hpp"
#include "../../libs/myteams/logging_client.h"

static void pack_fixed(std::vector<uint8_t> &buf, const std::string &s, size_t field_len)
{
    size_t copy_len = std::min(s.size(), field_len);
    for (size_t i = 0; i < copy_len; ++i)
        buf.push_back(static_cast<uint8_t>(s[i]));
    for (size_t i = copy_len; i < field_len; ++i)
        buf.push_back(0x00);
}


