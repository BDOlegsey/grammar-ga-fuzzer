// sweep.h — hyperparameter grid search
#pragma once

#include "ga.h"
#include "isut.h"
#include <string>
#include <vector>

struct SweepGrid {
    std::vector<int> population_sizes{20, 40, 80};
    std::vector<int> generation_counts{30, 60, 120};
    std::vector<double> mutation_rates{0.05, 0.12, 0.25};
    std::vector<double> crossover_rates{0.70, 0.85, 0.95};
    std::vector<double> alpha_values{0.35, 0.50, 0.65};
    int runs_per_config{10};
    std::uint32_t base_seed{42};
};

#include "cli_options.h"

void run_sweep(const std::vector<std::string>& sut_names,
               const SweepGrid& grid,
               const CLIOptions& opt);
