#pragma once

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <locale>
#include <memory>
#include <string>
#include <vector>

#pragma warning(disable : 4840)

class StringUtil
{
  public:
    template <typename... Args> static std::string Format(const std::string &format, Args... args)
    {
        size_t size =
            std::snprintf(nullptr, 0, format.c_str(), convert_for_snprintf(args)...) + 1; // Extra space for '\0'
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), convert_for_snprintf(args)...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

    static bool StartsWith(const std::string &value, const std::string &beginning)
    {
        if (beginning.size() > value.size())
        {
            return false;
        }

        return std::equal(beginning.begin(), beginning.end(), value.begin());
    }

    static bool EndsWith(const std::string &value, const std::string &ending)
    {
        if (ending.size() > value.size())
        {
            return false;
        }

        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    static std::vector<std::string> Split(const std::string &str, const std::string &delimiter)
    {
        // Skip delimiters at beginning.
        std::string::size_type lastPos = str.find_first_not_of(delimiter, 0);

        // Find first non-delimiter.
        std::string::size_type pos = str.find_first_of(delimiter, lastPos);

        std::vector<std::string> tokens;
        while (std::string::npos != pos || std::string::npos != lastPos)
        {
            // Found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos - lastPos));

            // Skip delimiters.
            lastPos = str.find_first_not_of(delimiter, pos);

            // Find next non-delimiter.
            pos = str.find_first_of(delimiter, lastPos);
        }

        return tokens;
    }

    static std::string ToLower(const std::string &str)
    {
        std::locale loc;
        std::string output = "";

        for (auto elem : str)
        {
            output += std::tolower(elem, loc);
        }

        return output;
    }

    static inline std::string Trim(const std::string &s)
    {
        std::string copy = s;
        ltrim(copy);
        rtrim(copy);
        return copy;
    }

  private:
    // trim from start (in place)
    static inline void ltrim(std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
    }

    // trim from end (in place)
    static inline void rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch) && ch != '\r' && ch != '\n'; })
                    .base(),
                s.end());
    }

    template <class T> static decltype(auto) convert_for_snprintf(T const &x)
    {
        return x;
    }

    static decltype(auto) convert_for_snprintf(std::string const &x)
    {
        return x.c_str();
    }
};
