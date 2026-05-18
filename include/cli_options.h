#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct CLIOptions {
    int runs = 20;
    int population_size = 40;
    int generations = 60;
    int samples_per_individual = 150;
    int elite_count = 3;
    int tournament_size = 3;
    double crossover_rate = 0.85;
    double mutation_rate = 0.12;
    double alpha = 0.50;
    double beta = 0.25;
    double gamma = 0.15;
    double delta = 0.10;
    int max_depth = 20;
    std::uint32_t seed = 42;
    std::size_t max_total_length = 256;
    std::string sut_name = "arithmetic";
    std::string mode = "default";
    std::vector<int> sweep_pop{20, 40, 80};
    std::vector<double> sweep_mut{0.05, 0.12, 0.25};
    bool log_weights = true;
};

[[nodiscard]] CLIOptions parse_options(int argc, char** argv);
