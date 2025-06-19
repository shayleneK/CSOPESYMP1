#include "AConsole.h"

AConsole::AConsole(String name) : name(name) {}

AConsole::String AConsole::getName() const
{
    return name;
}
