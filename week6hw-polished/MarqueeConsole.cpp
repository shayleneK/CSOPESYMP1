#include "MarqueeConsole.h"
#include <iostream>
#include <thread>
#include <chrono>

MarqueeConsole::MarqueeConsole() {
    this->name = "Marquee Console";
}

void MarqueeConsole::display() {
    std::cout << "\n=== Welcome to the Marquee Console ===" << std::endl;
    std::cout << "Type 'exit' to return to the main menu.\n" << std::endl;
}

void MarqueeConsole::handleInput(const std::string &input) {
    if (input == "exit") {
        this->requestExit();
    } else {
        for (char c : input) {
            std::cout << c << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << std::endl;
    }
}
