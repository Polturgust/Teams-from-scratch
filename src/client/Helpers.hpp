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

/**
 * @brief Append a fixed-width, zero-padded string field to a byte buffer.
 */
void        pack_fixed(std::vector<uint8_t> &buf, const std::string &s, size_t field_len);
/**
 * @brief Read a fixed-width field and trim trailing zero bytes.
 */
std::string read_fixed(const std::vector<uint8_t> &p, size_t &off, size_t len);
/**
 * @brief Read one big-endian uint32 field from payload and advance offset.
 */
uint32_t    read_u32(const std::vector<uint8_t> &p, size_t &off);
/**
 * @brief Read one uint8 field from payload and advance offset.
 */
uint8_t     read_u8(const std::vector<uint8_t> &p, size_t &off);
/**
 * @brief Read one big-endian int64 field from payload and advance offset.
 */
int64_t     read_i64(const std::vector<uint8_t> &p, size_t &off);

#endif // HELPERS_HPP
