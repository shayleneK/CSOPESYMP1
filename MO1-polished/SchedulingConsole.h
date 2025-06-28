#ifndef SCHEDULING_CONSOLE_H
#define SCHEDULING_CONSOLE_H

#include "AConsole.h"
#include "Scheduler.h"
#include "FCFSScheduler.h"
#include "RRScheduler.h"
#include <vector>
#include <memory>
#include <map>
#include <string>

class Process;
class Scheduler;

class SchedulingConsole : public AConsole
{
public:
    explicit SchedulingConsole(Scheduler *sched);

    void onEnabled() override;
    void display() override;
    void process(std::string &command) override;
    bool isRunning() const override;

private:
    Scheduler *scheduler;

    void clear_screen();
    void render_header();
    void render_footer();
    void render_running_processes(const std::vector<std::shared_ptr<Process>> &processes);
    void render_finished_processes(const std::vector<std::shared_ptr<Process>> &processes);
    // void render_cpu_utilization(const std::map<int, std::map<std::string, float>> &stats);
};

#endif // SCHEDULING_CONSOLE_H
