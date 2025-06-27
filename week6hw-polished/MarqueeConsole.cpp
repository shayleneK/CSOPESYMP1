#include "MarqueeConsole.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <conio.h>
#include <windows.h>

MarqueeConsole::MarqueeConsole() : AConsole("Marquee Console") {}

void MarqueeConsole::onEnabled() {
    exitFlag = false;
}

void MarqueeConsole::display() {
    run(); // Start the marquee animation loop
}

void MarqueeConsole::process(std::string &command) {
    if (command == "exit") {
        exitFlag = true;
    } else if (command == "clear") {
        commandHistory.clear();
    } else {
        commandHistory.push_back(command);
    }
}

bool MarqueeConsole::isRunning() const {
    return !exitFlag;
}

void MarqueeConsole::run() {
    exitFlag = false;
    x = 1; y = 2;
    dx = 1; dy = 1;
    input.clear();

    int width = getConsoleWidth();
    int height = getConsoleHeight();

    while (!exitFlag) {
        clearScreen();
        drawTitle();
        drawMarquee(x, y, marqueeText);
        drawInputArea(input);
        drawInputOutput();

        x += dx;
        y += dy;

        if (x <= 0 || x + static_cast<int>(marqueeText.length()) >= width)
            dx *= -1;
        if (y <= 1 || y >= height - 5)
            dy *= -1;

        if (_kbhit()) {
            char c = _getch();
            if (c == '\r') {
                if (input == "exit") {
                    exitFlag = true;
                } else if (input == "clear") {
                    commandHistory.clear();
                } else {
                    commandHistory.push_back(input);
                }
                input.clear();
            } else if (c == '\b') {
                if (!input.empty()) input.pop_back();
            } else if (isprint(c)) {
                input += c;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(REFRESH_DELAY));
    }
}

void MarqueeConsole::drawTitle() {
    COORD titlePos = {0, 0};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), titlePos);
    std::cout << "Displaying Marquee Console!";
}

void MarqueeConsole::drawMarquee(int x, int y, const std::string& text) {
    COORD pos = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    std::cout << text;
}

void MarqueeConsole::drawInputArea(const std::string& input) {
    int height = getConsoleHeight();
    COORD labelPos = {0, static_cast<SHORT>(height - 3)};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), labelPos);
    std::cout << "Input command in marquee console: ";

    COORD inputPos = {35, static_cast<SHORT>(height - 3)};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), inputPos);
    std::cout << "> " << input << " ";
}

void MarqueeConsole::drawInputOutput() {
    int height = getConsoleHeight();
    int y_pos = height - 2;
    for (auto it = commandHistory.rbegin(); it != commandHistory.rend() && y_pos < height; ++it, ++y_pos) {
        COORD labelPos = {0, static_cast<SHORT>(y_pos)};
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), labelPos);
        std::cout << "Command processed in marquee console: > ";

        COORD outputPos = {41, static_cast<SHORT>(y_pos)};
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), outputPos);
        std::cout << *it << "                ";
    }
}

void MarqueeConsole::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int MarqueeConsole::getConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

int MarqueeConsole::getConsoleHeight() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}