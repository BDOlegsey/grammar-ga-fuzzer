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

struct SweepRecord {
    std::string sut_name;
    int population_size{0};
    int generations{0};
    double mutation_rate{0.0};
    double crossover_rate{0.0};
    double alpha{0.0};
    std::uint32_t seed{0};
    double coverage_mean{0.0};
    double coverage_std{0.0};
    double bugs_mean{0.0};
    double bugs_std{0.0};
    double fitness_mean{0.0};
    double fitness_std{0.0};
    double runtime_mean{0.0};
};

class SweepCsvLogger {
public:
    explicit SweepCsvLogger(const std::string& path);
    void append(const SweepRecord& row);

private:
    std::ofstream out_;
};

class RuleWeightCsvLogger {
public:
    explicit RuleWeightCsvLogger(const std::string& path);
    void append(int run_id, int generation, const GenerationRecord& rec);

private:
    std::ofstream out_;
};

struct AblationRunRecord {
    std::string condition;
    std::uint32_t seed{0};
    double coverage{0.0};
    std::size_t unique_bugs{0};
    double best_fitness{0.0};
    double novelty_score{0.0};
    double diversity_score{0.0};
    double runtime_sec{0.0};
};

class AblationCsvLogger {
public:
    explicit AblationCsvLogger(const std::string& path);
    void append(const AblationRunRecord& row);

private:
    std::ofstream out_;
};

class AblationGenerationCsvLogger {
public:
    explicit AblationGenerationCsvLogger(const std::string& path);
    void append(const std::string& condition, const GenerationRecord& rec);

private:
    std::ofstream out_;
};
