// terminal_win.cpp
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream> // For std::ios_base

void setup_terminal()
{
    // Force Windows Console to use UTF-8 code page
    SetConsoleOutputCP(CP_UTF8);

    // Enable ANSI escape sequence processing on Windows host
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    std::ios_base::sync_with_stdio(false);
}
#else
void setup_terminal()
{
    // Non-Windows platforms don't need this initialization
    std::ios_base::sync_with_stdio(false);
}
#endif