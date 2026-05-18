// logger.cpp — CSV loggers for runs, generations, sweep, ablation, rule weights
#include "logger.h"
#include <iomanip>
#include <stdexcept>

RunCsvLogger::RunCsvLogger(const std::string& path)
    : out_(path, std::ios::out | std::ios::trunc) {
    if (!out_) throw std::runtime_error("Failed to open runs CSV file");
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
         << std::fixed << std::setprecision(2) << row.avg_input_length << '\n';
}

GenerationCsvLogger::GenerationCsvLogger(const std::string& path)
    : out_(path, std::ios::out | std::ios::trunc) {
    if (!out_) throw std::runtime_error("Failed to open generations CSV file");
    out_ << "run_id,generation,best_fitness,avg_fitness,best_coverage,best_bugs,"
            "cumulative_bugs,novelty,diversity,avg_input_length,avg_input_length_pop\n";
}

void GenerationCsvLogger::append(const GenerationRecord& row) {
    out_ << row.run_id << ','
         << row.generation << ','
         << std::fixed << std::setprecision(6) << row.best_fitness << ','
         << std::fixed << std::setprecision(6) << row.avg_fitness << ','
         << std::fixed << std::setprecision(6) << row.best_coverage << ','
         << row.best_bugs << ','
         << row.cumulative_bugs << ','
         << std::fixed << std::setprecision(6) << row.novelty << ','
         << std::fixed << std::setprecision(6) << row.diversity << ','
         << std::fixed << std::setprecision(2) << row.avg_input_length << ','
         << std::fixed << std::setprecision(2) << row.avg_input_length_pop << '\n';
}

SweepCsvLogger::SweepCsvLogger(const std::string& path)
    : out_(path, std::ios::out | std::ios::trunc) {
    if (!out_) throw std::runtime_error("Failed to open sweep CSV file");
    out_ << "sut_name,population_size,generations,mutation_rate,crossover_rate,alpha,"
            "seed,coverage_mean,coverage_std,bugs_mean,bugs_std,fitness_mean,fitness_std,runtime_mean\n";
}

void SweepCsvLogger::append(const SweepRecord& row) {
    out_ << row.sut_name << ','
         << row.population_size << ','
         << row.generations << ','
         << std::fixed << std::setprecision(6) << row.mutation_rate << ','
         << std::fixed << std::setprecision(6) << row.crossover_rate << ','
         << std::fixed << std::setprecision(6) << row.alpha << ','
         << row.seed << ','
         << std::fixed << std::setprecision(6) << row.coverage_mean << ','
         << std::fixed << std::setprecision(6) << row.coverage_std << ','
         << std::fixed << std::setprecision(6) << row.bugs_mean << ','
         << std::fixed << std::setprecision(6) << row.bugs_std << ','
         << std::fixed << std::setprecision(6) << row.fitness_mean << ','
         << std::fixed << std::setprecision(6) << row.fitness_std << ','
         << std::fixed << std::setprecision(6) << row.runtime_mean << '\n';
}

RuleWeightCsvLogger::RuleWeightCsvLogger(const std::string& path)
    : out_(path, std::ios::out | std::ios::trunc) {
    if (!out_) throw std::runtime_error("Failed to open rule weights CSV file");
    out_ << "run_id,generation,nonterminal,rule_index,probability\n";
}

void RuleWeightCsvLogger::append(int run_id, int generation, const GenerationRecord& rec) {
    if (!rec.has_weights) return;
    for (const auto& [nt, probs] : rec.best_rule_weights) {
        for (std::size_t i = 0; i < probs.size(); ++i) {
            out_ << run_id << ','
                 << generation << ','
                 << nt << ','
                 << i << ','
                 << std::fixed << std::setprecision(6) << probs[i] << '\n';
        }
    }
}

AblationCsvLogger::AblationCsvLogger(const std::string& path)
    : out_(path, std::ios::out | std::ios::trunc) {
    if (!out_) throw std::runtime_error("Failed to open ablation CSV file");
    out_ << "condition,seed,coverage,unique_bugs,best_fitness,novelty_score,diversity_score,runtime_sec\n";
}

void AblationCsvLogger::append(const AblationRunRecord& row) {
    out_ << row.condition << ','
         << row.seed << ','
         << std::fixed << std::setprecision(6) << row.coverage << ','
         << row.unique_bugs << ','
         << std::fixed << std::setprecision(6) << row.best_fitness << ','
         << std::fixed << std::setprecision(6) << row.novelty_score << ','
         << std::fixed << std::setprecision(6) << row.diversity_score << ','
         << std::fixed << std::setprecision(6) << row.runtime_sec << '\n';
}

AblationGenerationCsvLogger::AblationGenerationCsvLogger(const std::string& path)
    : out_(path, std::ios::out | std::ios::trunc) {
    if (!out_) throw std::runtime_error("Failed to open ablation generations CSV file");
    out_ << "condition,run_id,generation,best_fitness,avg_fitness,best_coverage,best_bugs,novelty,diversity\n";
}

void AblationGenerationCsvLogger::append(const std::string& condition, const GenerationRecord& rec) {
    out_ << condition << ','
         << rec.run_id << ','
         << rec.generation << ','
         << std::fixed << std::setprecision(6) << rec.best_fitness << ','
         << std::fixed << std::setprecision(6) << rec.avg_fitness << ','
         << std::fixed << std::setprecision(6) << rec.best_coverage << ','
         << rec.best_bugs << ','
         << std::fixed << std::setprecision(6) << rec.novelty << ','
         << std::fixed << std::setprecision(6) << rec.diversity << '\n';
}
