#pragma once

#include "AConsole.h"
#include <string>
#include <vector>

class MarqueeConsole : public AConsole {
public:
    MarqueeConsole();
    void onEnabled() override;
    void display() override;
    void process(std::string &command) override;
    bool isRunning() const override;

private:
    void run();
    void drawTitle();
    void drawMarquee(int x, int y, const std::string& text);
    void drawInputArea(const std::string& input);
    void drawInputOutput();
    void clearScreen();
    int getConsoleWidth();
    int getConsoleHeight();

    int x = 1, y = 2;
    int dx = 1, dy = 1;
    const std::string marqueeText = "Welcome to CSOPESY - Operating System Emulator";
    std::string input;
    bool exitFlag = false;
    std::vector<std::string> commandHistory;
};

// Constants for delays
static constexpr int REFRESH_DELAY = 10;  // screen refresh delay
static constexpr int POLLING_DELAY = 30;   // keyboard polling rate