#pragma once

#include "common.h"
#include "grammar.h"
#include <random>
#include <vector>

using Individual = Probabilities;

struct Candidate {
    Individual ind;
    double fitness{0.0};
    CorpusStats stats;
    std::array<std::size_t, kCoverageBranches> branch_hits{}; // per-branch hit counts
};

struct AdaptiveResult {
    RunSummary summary;
    std::vector<GenerationRecord> generation_log;
    Individual best_individual;
};

Individual random_individual(const Grammar& grammar, std::mt19937& rng);
Individual tournament_select(const std::vector<Candidate>& population, std::mt19937& rng, int tournament_size);
Individual crossover(const Individual& p1, const Individual& p2, std::mt19937& rng, double crossover_rate);
void mutate(Individual& ind, std::mt19937& rng, double mutation_rate);

RunSummary run_baseline_experiment(const Grammar& grammar, const GAConfig& cfg, int run_id);
AdaptiveResult run_adaptive_experiment(const Grammar& grammar, const GAConfig& cfg, int run_id);
