#pragma once

#include <iostream>
#include <string>
#include <mutex>

class IO
{
public:
    static void Out(const char* out)
    {
        std::unique_lock<std::mutex> lock(s_mutex);

        std::cout << out << std::endl;
    }

    static void Err(const char* err)
    {
        std::unique_lock<std::mutex> lock(s_mutex);

        std::cerr << err << std::endl;
    }

    static void Err(const char* msg, const std::exception& e)
    {
        std::unique_lock<std::mutex> lock(s_mutex);

        std::cerr << msg << std::endl << e.what() << std::endl;
    }

    static void Clear()
    {
        std::unique_lock<std::mutex> lock(s_mutex);

#ifdef _WIN32
        std::system("cls");
#else
        std::system("clear");
#endif
    }

    static void Flush()
    {
        std::unique_lock<std::mutex> lock(s_mutex);

        std::cout << std::flush;
    }

private:
    inline static std::mutex s_mutex{};
};