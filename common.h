#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>

// 60 coverage points: ~25 easy, ~15 medium, ~20 rare/specialized
constexpr std::size_t kCoverageBranches = 60;

struct ExecutionResult {
    std::array<bool, kCoverageBranches> branch_hits{};
    bool crashed{false};
    std::string crash_signature;
    std::size_t input_length{0};
};

struct CorpusStats {
    std::array<bool, kCoverageBranches> covered{};
    std::unordered_set<std::string> unique_crashes;
    std::size_t total_inputs{0};
    std::size_t total_length{0};      // sum of all input lengths
    std::size_t novelty_hits{0};       // newly discovered branches in this batch
};

struct RunSummary {
    std::string mode;
    std::uint32_t seed{0};
    std::size_t total_inputs{0};
    double coverage{0.0};
    std::size_t unique_bugs{0};
    double best_fitness{0.0};
    double runtime_sec{0.0};
    int generations{0};
    int population_size{0};
    int samples_per_individual{0};
    double mutation_rate{0.0};
    double crossover_rate{0.0};
    double novelty_score{0.0};         // average novelty per generation
    double diversity_score{0.0};       // final population diversity
    double avg_input_length{0.0};      // mean length of generated inputs
};

struct GenerationRecord {
    int run_id{0};
    int generation{0};
    double best_fitness{0.0};
    double avg_fitness{0.0};
    double best_coverage{0.0};
    std::size_t best_bugs{0};
    double novelty{0.0};               // newly discovered branches this generation
    double diversity{0.0};             // population diversity this generation
    double avg_input_length{0.0};      // average input length this generation
};

struct GAConfig {
    int population_size{40};
    int generations{60};
    int samples_per_individual{150};
    int elite_count{3};
    int tournament_size{3};
    double crossover_rate{0.85};
    double mutation_rate{0.12};
    double alpha{0.50};                // coverage weight
    double beta{0.25};                 // bug weight
    double gamma{0.15};                // novelty weight
    double delta{0.10};                // diversity weight
    int max_depth{20};
    std::uint32_t seed{42};
    std::size_t max_total_length{256}; // hard limit on generated input length
};
