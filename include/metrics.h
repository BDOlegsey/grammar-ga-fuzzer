#pragma once

#include "common.h"
#include <vector>

// Absorb execution result into corpus stats. Returns number of newly discovered branches.
std::size_t absorb(CorpusStats& stats, const ExecutionResult& result,
                   std::array<bool, kCoverageBranches>* globally_covered = nullptr);

double coverage_ratio(const CorpusStats& stats);
std::size_t bug_count(const CorpusStats& stats);
double normalized_bug_score(const CorpusStats& stats, std::size_t cap = 8);

// Novelty: how many new branches were discovered in this stats vs globally
std::size_t compute_novelty(const CorpusStats& stats,
                            const std::array<bool, kCoverageBranches>& globally_covered);

// Diversity: entropy of branch hit distribution across all inputs
double diversity_score(const std::array<std::size_t, kCoverageBranches>& branch_hit_counts);

// Average input length
double avg_input_length(const CorpusStats& stats);
