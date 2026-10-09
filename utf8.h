// SPDX-License-Identifier: MIT
#ifndef UTF8_H
#define UTF8_H

#include <cstddef>
#include <cstdint>
#include <string>

// Minimal UTF-8 <-> UTF-32 helpers shared by the native and WASM builds.

inline std::u32string Utf8ToUtf32(const char* text, std::size_t size)
{
    std::u32string result;
    for (std::size_t i = 0; i < size;)
    {
        unsigned char first = static_cast<unsigned char>(text[i]);
        char32_t codePoint = 0;
        int extraBytes = 0;
        if (first < 0x80)
        {
            codePoint = first;
        }
        else if ((first >> 5) == 0x6)
        {
            codePoint = first & 0x1F;
            extraBytes = 1;
        }
        else if ((first >> 4) == 0xE)
        {
            codePoint = first & 0x0F;
            extraBytes = 2;
        }
        else if ((first >> 3) == 0x1E)
        {
            codePoint = first & 0x07;
            extraBytes = 3;
        }
        else
        {
            ++i;
            continue;
        }
        if (i + (std::size_t)extraBytes >= size)
        {
            break;
        }
        for (int b = 0; b < extraBytes; b++)
        {
            unsigned char next = static_cast<unsigned char>(text[i + 1 + (std::size_t)b]);
            codePoint = (codePoint << 6) | (next & 0x3F);
        }
        result.push_back(codePoint);
        i += (std::size_t)extraBytes + 1;
    }
    return result;
}

inline void AppendUtf8(std::string& out, char32_t codePoint)
{
    if (codePoint < 0x80)
    {
        out.push_back((char)codePoint);
    }
    else if (codePoint < 0x800)
    {
        out.push_back((char)(0xC0 | (codePoint >> 6)));
        out.push_back((char)(0x80 | (codePoint & 0x3F)));
    }
    else if (codePoint < 0x10000)
    {
        out.push_back((char)(0xE0 | (codePoint >> 12)));
        out.push_back((char)(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (codePoint & 0x3F)));
    }
    else
    {
        out.push_back((char)(0xF0 | (codePoint >> 18)));
        out.push_back((char)(0x80 | ((codePoint >> 12) & 0x3F)));
        out.push_back((char)(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (codePoint & 0x3F)));
    }
}

inline std::string Utf8FromUtf32(const std::u32string& text)
{
    std::string out;
    out.reserve(text.size());
    for (char32_t codePoint : text)
    {
        AppendUtf8(out, codePoint);
    }
    return out;
}

#endif // UTF8_H
