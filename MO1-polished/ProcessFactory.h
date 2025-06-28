#pragma once

#include <memory>
#include <string>
#include "Process.h"

class ProcessFactory
{
public:
    static std::shared_ptr<Process> generate_dummy_process(const std::string &name, int min_ins, int max_ins);
};
