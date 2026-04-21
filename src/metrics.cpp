#include "metrics.h"
#include <algorithm>
#include <cmath>

std::size_t absorb(CorpusStats& stats, const ExecutionResult& result,
                   std::array<bool, kCoverageBranches>* globally_covered) {
    stats.total_inputs++;
    stats.total_length += result.input_length;

    std::size_t newly_found = 0;
    for (std::size_t i = 0; i < kCoverageBranches; ++i) {
        if (result.branch_hits[i]) {
            if (globally_covered && !(*globally_covered)[i]) {
                (*globally_covered)[i] = true;
                newly_found++;
            }
            stats.covered[i] = true;
        }
    }
    stats.novelty_hits += newly_found;

    if (result.crashed && !result.crash_signature.empty()) {
        stats.unique_crashes.insert(result.crash_signature);
    }
    return newly_found;
}

double coverage_ratio(const CorpusStats& stats) {
    std::size_t covered = 0;
    for (bool b : stats.covered) {
        if (b) covered++;
    }
    return static_cast<double>(covered) / static_cast<double>(kCoverageBranches);
}

std::size_t bug_count(const CorpusStats& stats) {
    return stats.unique_crashes.size();
}

double normalized_bug_score(const CorpusStats& stats, std::size_t cap) {
    if (cap == 0) return 0.0;
    double score = static_cast<double>(bug_count(stats)) / static_cast<double>(cap);
    return std::min(1.0, score);
}

std::size_t compute_novelty(const CorpusStats& stats,
                            const std::array<bool, kCoverageBranches>& globally_covered) {
    std::size_t novel = 0;
    for (std::size_t i = 0; i < kCoverageBranches; ++i) {
        if (stats.covered[i] && !globally_covered[i]) {
            novel++;
        }
    }
    return novel;
}

double diversity_score(const std::array<std::size_t, kCoverageBranches>& branch_hit_counts) {
    std::size_t total = 0;
    for (auto c : branch_hit_counts) total += c;
    if (total == 0) return 0.0;

    double entropy = 0.0;
    for (auto c : branch_hit_counts) {
        if (c == 0) continue;
        double p = static_cast<double>(c) / static_cast<double>(total);
        entropy -= p * std::log2(p);
    }
    // Normalize by log2 of number of branches (max entropy)
    double max_ent = std::log2(static_cast<double>(kCoverageBranches));
    return max_ent > 0.0 ? entropy / max_ent : 0.0;
}

double avg_input_length(const CorpusStats& stats) {
    if (stats.total_inputs == 0) return 0.0;
    return static_cast<double>(stats.total_length) / static_cast<double>(stats.total_inputs);
}
