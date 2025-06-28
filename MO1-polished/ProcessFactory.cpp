#include "ProcessFactory.h"
#include "Command.h"

#include <random>
#include <sstream>

std::shared_ptr<Process> ProcessFactory::generate_dummy_process(const std::string &name, int min_ins, int max_ins)
{
    auto process = std::make_shared<Process>(name);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ins_dist(min_ins, max_ins);
    std::uniform_int_distribution<> op_dist(0, 5);

    int instruction_count = ins_dist(gen);

    for (int i = 0; i < instruction_count; ++i)
    {
        int op = op_dist(gen);
        switch (op)
        {
        case 0:
        {
            std::uniform_int_distribution<> msg_type(0, 1);
            std::vector<std::string> vars = {"x", "y", "result", "temp", "value"};
            std::string message = (msg_type(gen) == 0)
                                      ? "Hello world from " + name + "!"
                                      : "\"Value from: \" + " + vars[gen() % vars.size()];
            process->add_command(std::make_shared<PrintCommand>(message));
            break;
        }
        case 1:
        case 2:
        {
            std::vector<std::string> vars = {"a", "b", "c", "temp", "sum", "diff"};
            std::string target = vars[gen() % vars.size()];
            std::string op1 = vars[gen() % vars.size()];
            std::string op2 = vars[gen() % vars.size()];
            bool op1_is_var = gen() % 2;
            bool op2_is_var = gen() % 2;
            std::uniform_int_distribution<uint16_t> val_dist(1, 500);
            uint16_t val1 = val_dist(gen), val2 = val_dist(gen);
            if (op == 1)
                process->add_command(std::make_shared<AddCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
            else
                process->add_command(std::make_shared<SubtractCommand>(target, op1, op2, op1_is_var, op2_is_var, val1, val2));
            break;
        }
        case 3:
        {
            std::vector<std::string> vars = {"a1", "b1", "c1", "temp1", "varX", "tempVar"};
            std::string var = vars[gen() % vars.size()];
            std::uniform_int_distribution<uint16_t> val_dist(0, 1000);
            uint16_t value = val_dist(gen);
            process->add_command(std::make_shared<DeclareCommand>(var, value));
            break;
        }
        case 4:
        {
            std::uniform_int_distribution<> rep_dist(2, 5);
            int repeats = rep_dist(gen);
            std::vector<std::shared_ptr<Command>> nested_cmds;
            std::uniform_int_distribution<> nested_len_dist(1, 3);
            int nested_instruction_count = nested_len_dist(gen);
            std::uniform_int_distribution<> nested_op_dist(0, 3);
            for (int j = 0; j < nested_instruction_count; ++j)
            {
                int inner_op = nested_op_dist(gen);
                switch (inner_op)
                {
                case 0:
                    nested_cmds.push_back(std::make_shared<PrintCommand>("\"Loop message " + std::to_string(j) + "\""));
                    break;
                case 1:
                {
                    std::string target = "loop" + std::to_string(j);
                    std::string op1 = "a" + std::to_string(j);
                    std::string op2 = "b" + std::to_string(j);
                    uint16_t val2 = gen() % 50 + 1;
                    nested_cmds.push_back(std::make_shared<AddCommand>(target, op1, op2, true, false, 0, val2));
                    break;
                }
                case 2:
                {
                    std::string var = "d" + std::to_string(j);
                    uint16_t val = gen() % 100;
                    nested_cmds.push_back(std::make_shared<DeclareCommand>(var, val));
                    break;
                }
                case 3:
                    nested_cmds.push_back(std::make_shared<SleepCommand>(1));
                    break;
                }
            }
            process->add_command(std::make_shared<ForCommand>(nested_cmds, repeats));
            break;
        }
        case 5:
        {
            std::uniform_int_distribution<uint8_t> sleep_ticks(10, 100);
            uint8_t ticks = sleep_ticks(gen);
            process->add_command(std::make_shared<SleepCommand>(ticks));
            break;
        }
        default:
            process->add_command(std::make_shared<PrintCommand>("\"Unknown operation fallback from " + name + "\""));
            break;
        }
    }

    return process;
}
