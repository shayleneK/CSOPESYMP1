#pragma once

#include "AConsole.h"
#include "Process.h"

#include <string>
#include <ctime>
#include <memory>

class ScreenConsole : public AConsole
{
public:
    explicit ScreenConsole(const std::string &name);

    void clearConsole() const;
    void draw() const;
    void onEnabled() override;
    void display() override;
    bool isRunning() const override;
    std::shared_ptr<Process> attachedProcess;

    void process(std::string &command) override;
    void attachProcess(std::shared_ptr<Process> process);
    std::shared_ptr<Process> getAttachedProcess() const;

private:
    std::string m_name;
    const int m_lineNumber;
    const int m_totalLines;
    std::time_t m_createdTime;
};
