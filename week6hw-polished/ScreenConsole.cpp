#include "ScreenConsole.h"
#include "ConsoleManager.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

ScreenConsole::ScreenConsole(const std::string &name)
    : AConsole("screen"),
      m_name(name),
      m_lineNumber(1),
      m_totalLines(100),
      m_createdTime(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
{
}

void ScreenConsole::clearConsole() const
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void ScreenConsole::draw() const
{
    clearConsole();
    std::tm buf{};
    localtime_s(&buf, &m_createdTime);

    std::cout << "\n--- Screen: " << m_name << " ---\n";
    std::cout << "Instruction: " << m_lineNumber << " / " << m_totalLines << "\n";
    std::cout << "Created at: "
              << std::put_time(&buf, "%m/%d/%Y, %I:%M:%S %p") << "\n";
    std::cout << "Type 'exit' to return to main menu.\n";
}

void ScreenConsole::display()
{
    draw();
}
void ScreenConsole::process(std::string &command)
{
    // if (command == "exit")
    // {
    //     ConsoleManager::getInstance()->switchConsole(MAIN_CONSOLE);
    //     // return true;
    // }
    // else
    // {
    //     std::cout << "Unknown command: " << command << "\n";
    // }

    // return false;
}

void ScreenConsole::onEnabled()
{
    std::cout << "+--------------------------------------------------+\n";
    std::cout << "| Screen Console is now active.                    |\n";
    std::cout << "+--------------------------------------------------+\n";
}

bool ScreenConsole::isRunning() const
{
    return false;
}
