/*
** EPITECH PROJECT, 2025
** G-NWP-400-NCE-4-1-myteams-8
** File description:
** helpers.cpp
*/

#include "Helpers.hpp"

void pack_fixed(std::vector<uint8_t> &buf, const std::string &s, size_t field_len)
{
    size_t copy_len = std::min(s.size(), field_len);
    for (size_t i = 0; i < copy_len; ++i)
        buf.push_back(static_cast<uint8_t>(s[i]));
    for (size_t i = copy_len; i < field_len; ++i)
        buf.push_back(0x00);
}

std::string read_fixed(const std::vector<uint8_t> &p, size_t &off, size_t len)
{
    if (off + len > p.size()) return "";
    std::string s(reinterpret_cast<const char *>(p.data() + off), len);
    size_t end = s.find('\0');
    if (end != std::string::npos) s.resize(end);
    off += len;
    return s;
}

uint32_t read_u32(const std::vector<uint8_t> &p, size_t &off)
{
    if (off + 4 > p.size()) return 0;
    uint32_t v;
    std::memcpy(&v, p.data() + off, 4);
    off += 4;
    return ntohl(v);
}

uint8_t read_u8(const std::vector<uint8_t> &p, size_t &off)
{
    if (off >= p.size()) return 0;
    return p[off++];
}

int64_t read_i64(const std::vector<uint8_t> &p, size_t &off)
{
    if (off + 8 > p.size()) return 0;
    int64_t v;
    std::memcpy(&v, p.data() + off, 8);
    off += 8;
    uint64_t u;
    std::memcpy(&u, &v, 8);
    u = ((u & 0x00000000000000FFULL) << 56)
      | ((u & 0x000000000000FF00ULL) << 40)
      | ((u & 0x0000000000FF0000ULL) << 24)
      | ((u & 0x00000000FF000000ULL) <<  8)
      | ((u & 0x000000FF00000000ULL) >>  8)
      | ((u & 0x0000FF0000000000ULL) >> 24)
      | ((u & 0x00FF000000000000ULL) >> 40)
      | ((u & 0xFF00000000000000ULL) >> 56);
    std::memcpy(&v, &u, 8);
    return v;
}
