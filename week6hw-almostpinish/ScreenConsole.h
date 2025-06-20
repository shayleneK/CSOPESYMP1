#ifndef SCREEN_CONSOLE_H
#define SCREEN_CONSOLE_H

#include "AConsole.h"
#include "Process.h"
#include <memory>

class ScreenConsole : public AConsole {
private:
    std::shared_ptr<Process> process;

public:
    explicit ScreenConsole(std::shared_ptr<Process> p);

    void onEnabled() override;
    void display() override;
    void process() override;
    bool isRunning() const override { return !process->is_finished; }
};