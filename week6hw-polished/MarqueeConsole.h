#ifndef MARQUEE_CONSOLE_H
#define MARQUEE_CONSOLE_H

#include "AConsole.h"

class MarqueeConsole : public AConsole {
public:
    MarqueeConsole();
    void display() override;
    void handleInput(const std::string &input) override;
};

#endif // MARQUEE_CONSOLE_H
