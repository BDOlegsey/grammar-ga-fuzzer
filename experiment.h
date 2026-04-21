#pragma once

#include "ga.h"
#include <vector>

// Build a rich grammar with unary ops, functions, variables, hex, floats, ternary
Grammar build_rich_grammar();

void run_experiments(
    const Grammar& grammar,
    const GAConfig& cfg,
    int runs,
    std::vector<RunSummary>& baseline_runs,
    std::vector<RunSummary>& adaptive_runs,
    std::vector<GenerationRecord>& generation_logs
);
