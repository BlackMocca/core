/*
 * ThaiWordBreaker.cpp
 * See ThaiWordBreaker.h for description.
 */

#include "ThaiWordBreaker.h"

#include <fstream>
#include <sstream>
#include <algorithm>

// UTF-8 → wstring conversion (portable, no ICU/codecvt dependency)
// Works on Windows (Win32 API) and all POSIX platforms (manual decode).
#ifdef _WIN32
#  include <windows.h>
static std::wstring Utf8ToWstring(const std::string& s)
{
    if (s.empty()) return L"";
    int needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring result(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &result[0], needed);
    return result;
}
#else
// Manual UTF-8 decode: handles all BMP characters (sufficient for Thai U+0E00–U+0E7F).
// Avoids std::codecvt_utf8 which is deprecated in C++17 and removed in some C++20 builds.
static std::wstring Utf8ToWstring(const std::string& s)
{
    std::wstring result;
    result.reserve(s.size());
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.c_str());
    const unsigned char* end = p + s.size();
    while (p < end)
    {
        unsigned char c = *p;
        uint32_t cp = 0;
        int extra = 0;
        if (c < 0x80)
        {
            cp = c; extra = 0;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            cp = c & 0x1F; extra = 1;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            cp = c & 0x0F; extra = 2;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            cp = c & 0x07; extra = 3;
        }
        else
        {
            ++p; continue; // invalid lead byte, skip
        }
        ++p;
        for (int i = 0; i < extra && p < end; ++i, ++p)
        {
            unsigned char cont = *p;
            if ((cont & 0xC0) != 0x80) { cp = 0xFFFD; break; } // invalid continuation
            cp = (cp << 6) | (cont & 0x3F);
        }
        if (sizeof(wchar_t) == 4)
        {
            result += static_cast<wchar_t>(cp);
        }
        else // UTF-16 (Windows wchar_t = 2 bytes, but handled above via Win32 API)
        {
            if (cp < 0x10000)
                result += static_cast<wchar_t>(cp);
            // surrogate pair for > 0xFFFF: Thai chars are all < 0x10000, skip for simplicity
        }
    }
    return result;
}
#endif

namespace BinDocxRW
{

ThaiWordBreaker::ThaiWordBreaker()
{
}

bool ThaiWordBreaker::Init(const std::string& sDictPath)
{
    m_dict.clear();

    std::ifstream ifs(sDictPath);
    if (!ifs.is_open())
        return false;

    std::string line;
    while (std::getline(ifs, line))
    {
        // Strip trailing CR/LF/spaces
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        if (line.empty())
            continue;

        std::wstring word = Utf8ToWstring(line);
        if (!word.empty())
            m_dict.insert(word);
    }

    return !m_dict.empty();
}

bool ThaiWordBreaker::IsLoaded() const
{
    return !m_dict.empty();
}

/*static*/ bool ThaiWordBreaker::IsThai(wchar_t c)
{
    return c >= 0x0E00 && c <= 0x0E7F;
}

std::vector<std::wstring> ThaiWordBreaker::SegmentThaiSequence(const std::wstring& sText,
                                                                int nStart,
                                                                int nEnd) const
{
    std::vector<std::wstring> result;
    int i = nStart;

    while (i < nEnd)
    {
        bool matched = false;

        // Maximum matching: try longest possible word first
        int maxLen = std::min(MAX_WORD_LEN, nEnd - i);
        for (int len = maxLen; len > 1; --len)
        {
            std::wstring candidate = sText.substr(i, len);
            if (m_dict.count(candidate))
            {
                result.push_back(candidate);
                i += len;
                matched = true;
                break;
            }
        }

        if (!matched)
        {
            // No dictionary match: advance one character
            result.push_back(sText.substr(i, 1));
            ++i;
        }
    }

    return result;
}

std::vector<std::wstring> ThaiWordBreaker::Segment(const std::wstring& sText) const
{
    std::vector<std::wstring> result;
    int n = (int)sText.size();
    int i = 0;

    while (i < n)
    {
        if (IsThai(sText[i]))
        {
            // Collect contiguous Thai characters
            int thaiStart = i;
            while (i < n && IsThai(sText[i]))
                ++i;

            // Segment the Thai run
            std::vector<std::wstring> words = SegmentThaiSequence(sText, thaiStart, i);
            for (auto& w : words)
                result.push_back(w);
        }
        else
        {
            // Non-Thai: emit as-is (individual char or accumulate non-Thai run)
            int nonThaiStart = i;
            while (i < n && !IsThai(sText[i]))
                ++i;
            result.push_back(sText.substr(nonThaiStart, i - nonThaiStart));
        }
    }

    return result;
}

} // namespace BinDocxRW
