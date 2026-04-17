/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** Helpers.hpp
*/

// y'a un monde c'est du c style

#ifndef HELPERS_HPP
    #define HELPERS_HPP

    #include <string>
    #include <vector>
    #include <cstdint>
    #include <cstring>
    #include <arpa/inet.h>


void        pack_fixed(std::vector<uint8_t> &buf, const std::string &s, size_t field_len);
std::string read_fixed(const std::vector<uint8_t> &p, size_t &off, size_t len);
uint32_t    read_u32(const std::vector<uint8_t> &p, size_t &off);
uint8_t     read_u8(const std::vector<uint8_t> &p, size_t &off);
int64_t     read_i64(const std::vector<uint8_t> &p, size_t &off);

#endif // HELPERS_HPP