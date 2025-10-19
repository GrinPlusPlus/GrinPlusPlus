#pragma once

#include <iostream>
#include <string>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

class IO
{
public:
    static void MakeHeadless()
    {
        s_headless = true;
    }

    static void Out(const char* out)
    {
        if (s_headless) { return; }

        std::unique_lock<std::mutex> lock(s_mutex);
        std::cout << out << std::endl;
    }

    static void Err(const char* err)
    {
        if (s_headless) { return; }

        std::unique_lock<std::mutex> lock(s_mutex);
        std::cerr << err << std::endl;
    }

    static void Err(const char* msg, const std::exception& e)
    {
        if (s_headless) { return; }

        std::unique_lock<std::mutex> lock(s_mutex);
        std::cerr << msg << std::endl << e.what() << std::endl;
    }

    static void Clear()
    {
        if (s_headless) { return; }

        std::unique_lock<std::mutex> lock(s_mutex);

        cls();
    }

    static void Flush()
    {
        std::unique_lock<std::mutex> lock(s_mutex);
        std::cout << std::flush;
    }

private:
    inline static std::atomic_bool s_headless{ false };
    inline static std::mutex s_mutex{};

#ifdef _WIN32
    static void cls(void)
    {
        DWORD n;                         /* Number of characters written */
        DWORD size;                      /* number of visible characters */
        COORD coord = { 0 };               /* Top left screen position */
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        /* Get a handle to the console */
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

        GetConsoleScreenBufferInfo(h, &csbi);

        /* Find the number of characters to overwrite */
        size = csbi.dwSize.X * csbi.dwSize.Y;

        /* Overwrite the screen buffer with whitespace */
        FillConsoleOutputCharacter(h, TEXT(' '), size, coord, &n);
        GetConsoleScreenBufferInfo(h, &csbi);
        FillConsoleOutputAttribute(h, csbi.wAttributes, size, coord, &n);

        /* Reset the cursor to the top left position */
        SetConsoleCursorPosition(h, coord);
    }
#else
    static void cls(void)
    {
        std::cout << "\033[2J\033[;H";
    }
#endif
};