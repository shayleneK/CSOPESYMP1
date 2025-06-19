#ifndef MAIN_CONSOLE_H
#define MAIN_CONSOLE_H

#include "AConsole.h"
#include "Scheduler.h"

class MainConsole : public AConsole
{
private:
    Scheduler *scheduler;
    bool running;

public:
    explicit MainConsole(Scheduler *sched);

    void onEnabled() override;
    void display() override;
    void process() override;

    // Implement pure virtual function from AConsole
    bool isRunning() const override;
    void clear_screen();
};

#endif // MAIN_CONSOLE_H