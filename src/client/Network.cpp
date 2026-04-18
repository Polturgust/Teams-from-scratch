/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Network.cpp
*/

#include "Client.hpp"

void Client::_handle_server_read()
{
    uint8_t tmp[4096];
    ssize_t n = read(_fd, tmp, sizeof(tmp));
    if (n <= 0) {
        std::cerr << "Server disconnected." << std::endl;
        _running = false;
        return;
    }
    _recv_buf.insert(_recv_buf.end(), tmp, tmp + n);
    _process_recv_buffer();
}

void Client::_handle_server_write()
{
    if (_send_buf.empty()) return;
    ssize_t n = write(_fd, _send_buf.data(), _send_buf.size());
    if (n <= 0) return;
    _send_buf.erase(_send_buf.begin(), _send_buf.begin() + n);
}

void Client::_process_recv_buffer()
{
    const size_t HEADER = 6;
    while (_recv_buf.size() >= HEADER) {
        uint16_t code;
        uint32_t psize;
        std::memcpy(&code,  _recv_buf.data(),     2);
        std::memcpy(&psize, _recv_buf.data() + 2, 4);
        code  = ntohs(code);
        psize = ntohl(psize);

        if (_recv_buf.size() < HEADER + psize) break; // wait for full payload

        std::vector<uint8_t> payload(
            _recv_buf.begin() + HEADER,
            _recv_buf.begin() + HEADER + psize);
        _recv_buf.erase(_recv_buf.begin(), _recv_buf.begin() + HEADER + psize);

        _handle_response(code, payload);
    }
}
