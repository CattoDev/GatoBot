#include <Windows.h>

#include <MinHook.h>

#ifndef GB_GEODE
#include <cocos2d.h>
using namespace cocos2d;
#endif

#include <gd.h>

#include <subprocess.hpp>

#undef snprintf
#include <json.hpp>

// std
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <fstream>

/*
    GD's version of std::string
*/
struct GDH_DLL gdstring {
    union
    {
        char inline_data[16];
        char *ptr;
    } m_data;
    size_t m_size = 0;
    size_t m_capacity = 15;

    gdstring(const char *data, size_t size)
    {
        reinterpret_cast<void *(__thiscall *)(void *, const char *, size_t)>(gd::base + 0xf840)(this, data, size);
    }

    explicit gdstring(const std::string_view &str) : gdstring(str.data(), str.size()) {}
    gdstring(const char *str) : gdstring(std::string_view(str)) {}
    gdstring(const std::string &str) : gdstring(str.c_str(), str.size()) {}

    size_t size() const { return m_size; }

    gdstring &operator=(const std::string &other)
    {
        if (m_capacity > 15)
            delete m_data.ptr;
        reinterpret_cast<void *(__thiscall *)(void *, const char *, size_t)>(gd::base + 0xf840)(this, other.c_str(), other.size());
        return *this;
    }

    const char *c_str() const
    {
        if (m_capacity <= 15)
            return m_data.inline_data;
        return m_data.ptr;
    }

    std::string_view sv() const
    {
        return std::string_view(c_str(), m_size);
    }

    operator std::string() const { return std::string(sv()); }
};