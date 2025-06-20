#ifndef MAINCONSOLE_H
#define MAINCONSOLE_H

#include "AConsole.h"
#include <string>

class MainConsole : public AConsole
{
public:
    MainConsole() = default;
    ~MainConsole();

    void display() override;
    void process(std::string &command) override;

private:
    void printHeader() const;
    void clearConsole() const;
};

#endif // MAINCONSOLE_H
