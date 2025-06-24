#pragma once

#include "AConsole.h"
#include <string>
#include <ctime>

class ScreenConsole : public AConsole
{
public:
    explicit ScreenConsole(const std::string &name);

    void clearConsole() const;
    void draw() const;
    void onEnabled() override;
    void display() override;
    bool isRunning() const override;

    void process(std::string &command) override;

private:
    std::string m_name;
    const int m_lineNumber;
    const int m_totalLines;
    std::time_t m_createdTime;
};
