/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Packet.cpp
*/

#include "Client.hpp"

void Client::_send_packet(uint16_t cmd, const void *payload, uint32_t size)
{
    uint16_t cmd_net  = htons(cmd);
    uint32_t size_net = htonl(size);

    uint8_t header[6];
    std::memcpy(header,     &cmd_net,  2);
    std::memcpy(header + 2, &size_net, 4);

    _send_buf.insert(_send_buf.end(), header, header + 6);
    if (payload && size > 0)
        _send_buf.insert(_send_buf.end(),
            static_cast<const uint8_t *>(payload),
            static_cast<const uint8_t *>(payload) + size);

    _handle_server_write();
}
