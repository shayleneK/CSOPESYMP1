#ifndef A_CONSOLE_H
#define A_CONSOLE_H

#include <string>

class AConsole
{
protected:
    std::string name;

public:
    typedef std::string String;

    AConsole(String name);
    virtual ~AConsole() = default;

    virtual void onEnabled() = 0;
    virtual void display() = 0;
    virtual void process(std::string &command) = 0;
    virtual bool isRunning() const = 0;

    friend class ConsoleManager;
};

#endif // A_CONSOLE_H