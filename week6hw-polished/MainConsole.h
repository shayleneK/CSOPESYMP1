#ifndef MAINCONSOLE_H
#define MAINCONSOLE_H

#include "AConsole.h"
#include <string>

class MainConsole : public AConsole
{
public:
    MainConsole();

    void display() override;
    void process(std::string &command) override;
    void onEnabled() override;
    bool isRunning() const override;

private:
    void printHeader() const;
    void clearConsole() const;
};

#endif // MAINCONSOLE_H
