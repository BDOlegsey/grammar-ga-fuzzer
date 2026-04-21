#include "logger.h"
#include <iomanip>
#include <stdexcept>

RunCsvLogger::RunCsvLogger(const std::string& path)
    : out_(path, std::ios::out | std::ios::trunc) {
    if (!out_) {
        throw std::runtime_error("Failed to open runs CSV file");
    }

    out_ << "mode,seed,total_inputs,coverage,unique_bugs,best_fitness,runtime_sec,"
            "generations,population_size,samples_per_individual,mutation_rate,crossover_rate,"
            "novelty_score,diversity_score,avg_input_length\n";
}

void RunCsvLogger::append(const RunSummary& row) {
    out_ << row.mode << ','
         << row.seed << ','
         << row.total_inputs << ','
         << std::fixed << std::setprecision(6) << row.coverage << ','
         << row.unique_bugs << ','
         << std::fixed << std::setprecision(6) << row.best_fitness << ','
         << std::fixed << std::setprecision(6) << row.runtime_sec << ','
         << row.generations << ','
         << row.population_size << ','
         << row.samples_per_individual << ','
         << std::fixed << std::setprecision(6) << row.mutation_rate << ','
         << std::fixed << std::setprecision(6) << row.crossover_rate << ','
         << std::fixed << std::setprecision(6) << row.novelty_score << ','
         << std::fixed << std::setprecision(6) << row.diversity_score << ','
         << std::fixed << std::setprecision(2) << row.avg_input_length
         << '\n';
}

GenerationCsvLogger::GenerationCsvLogger(const std::string& path)
    : out_(path, std::ios::out | std::ios::trunc) {
    if (!out_) {
        throw std::runtime_error("Failed to open generations CSV file");
    }

    out_ << "run_id,generation,best_fitness,avg_fitness,best_coverage,best_bugs,"
            "novelty,diversity,avg_input_length\n";
}

void GenerationCsvLogger::append(const GenerationRecord& row) {
    out_ << row.run_id << ','
         << row.generation << ','
         << std::fixed << std::setprecision(6) << row.best_fitness << ','
         << std::fixed << std::setprecision(6) << row.avg_fitness << ','
         << std::fixed << std::setprecision(6) << row.best_coverage << ','
         << row.best_bugs << ','
         << std::fixed << std::setprecision(6) << row.novelty << ','
         << std::fixed << std::setprecision(6) << row.diversity << ','
         << std::fixed << std::setprecision(2) << row.avg_input_length
         << '\n';
}
