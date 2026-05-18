#pragma once

#include "common.h"
#include <vector>

std::size_t absorb(CorpusStats& stats, const ExecutionResult& result,
                   std::array<bool, kMaxBranches>* globally_covered = nullptr);

[[nodiscard]] double coverage_ratio(const CorpusStats& stats);
[[nodiscard]] std::size_t bug_count(const CorpusStats& stats);
[[nodiscard]] double normalized_bug_score(const CorpusStats& stats, std::size_t cap = 8);

[[nodiscard]] std::size_t compute_novelty(const CorpusStats& stats,
                                        const std::array<bool, kMaxBranches>& globally_covered);

[[nodiscard]] double diversity_score(const std::array<std::size_t, kMaxBranches>& branch_hit_counts,
                                     std::size_t num_branches);

[[nodiscard]] double avg_input_length(const CorpusStats& stats);
