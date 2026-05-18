#pragma once

#include "isut.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

constexpr std::size_t kCoverageBranches = 60;

struct CorpusStats {
    std::array<bool, kMaxBranches> covered{};
    std::unordered_set<std::string> unique_crashes;
    std::size_t total_inputs{0};
    std::size_t total_length{0};
    std::size_t novelty_hits{0};
    std::size_t num_branches{0};
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
    double novelty_score{0.0};
    double diversity_score{0.0};
    double avg_input_length{0.0};
};

struct GenerationRecord {
    int run_id{0};
    int generation{0};
    double best_fitness{0.0};
    double avg_fitness{0.0};
    double best_coverage{0.0};
    std::size_t best_bugs{0};
    std::size_t cumulative_bugs{0};
    double novelty{0.0};
    double diversity{0.0};
    double avg_input_length{0.0};
    double avg_input_length_pop{0.0};
    std::map<std::string, std::vector<double>> best_rule_weights;
    bool has_weights{false};
};

struct GAConfig {
    int population_size{40};
    int generations{60};
    int samples_per_individual{150};
    int elite_count{3};
    int tournament_size{3};
    double crossover_rate{0.85};
    double mutation_rate{0.12};
    double alpha{0.50};
    double beta{0.25};
    double gamma{0.15};
    double delta{0.10};
    int max_depth{20};
    std::uint32_t seed{42};
    std::size_t max_total_length{256};
};
