#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <map>
#include <string>

// Forward declarations
class Process;

/**
 * @brief The Scheduler class manages a First-Come-First-Served (FCFS)
 *        scheduling algorithm with support for multiple CPU cores.
 */
class Scheduler
{
protected:
    std::vector<std::thread> cpu_cores;                  // Worker threads simulating CPU cores
    std::queue<std::shared_ptr<Process>> ready_queue;    // Shared ready queue
    std::mutex queue_mutex;                              // Mutex for queue access
    std::condition_variable queue_condition;             // Signal when new process arrives
    std::vector<bool> core_available;                    // Track which cores are idle
    std::vector<std::shared_ptr<Process>> all_processes; // All processes created
    std::map<int, std::shared_ptr<Process>> current_processes; // Currently running processes
    std::mutex running_mutex;
    bool running = true;                                 // Scheduler state

    int total_cpu_time = 0;                // Total simulated CPU time
    std::map<int, int> core_process_count; // Number of processes handled per core
    std::map<int, int> core_util_time;     // Time each core was busy

public:
    explicit Scheduler(int num_cores);
    ~Scheduler();

    /**
     * @brief Adds a new process to the ready queue
     * @param process Shared pointer to the process
     */
    void add_process(std::shared_ptr<Process> process);

    /**
     * @brief Starts the scheduler and CPU worker threads
     */
    void start();

    /**
     * @brief Shuts down the scheduler and worker threads
     */
    void shutdown();

    /**
     * @brief Gets list of currently running processes
     * @return Vector of shared pointers to running processes
     */
    std::vector<std::shared_ptr<Process>> get_running_processes();

    /**
     * @brief Gets list of finished processes
     * @return Vector of shared pointers to finished processes
     */
    std::vector<std::shared_ptr<Process>> get_finished_processes();

    /**
     * @brief Gets CPU utilization stats
     * @return Map of CPU core stats
     */
    std::map<int, std::map<std::string, float>> get_cpu_stats();

    /**
     * @brief Checks if all processes have been completed
     * @return True if done, false otherwise
     */
    bool is_done();

    /**
     * @brief Gets all processes added to the scheduler
     * @return Vector of shared pointers to all processes
     */
    std::vector<std::shared_ptr<Process>> get_all_processes();

private:
    /**
     * @brief Worker function for CPU core threads
     * @param core_id Logical ID of the CPU core
     */
    virtual void run_core(int core_id);
};

#endif // SCHEDULER_H