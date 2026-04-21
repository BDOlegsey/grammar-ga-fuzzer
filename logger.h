#pragma once

#include "common.h"
#include <fstream>
#include <string>

class RunCsvLogger {
public:
    explicit RunCsvLogger(const std::string& path);
    void append(const RunSummary& row);

private:
    std::ofstream out_;
};

class GenerationCsvLogger {
public:
    explicit GenerationCsvLogger(const std::string& path);
    void append(const GenerationRecord& row);

private:
    std::ofstream out_;
};
