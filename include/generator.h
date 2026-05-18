#pragma once

#include "grammar.h"
#include "common.h"
#include <random>
#include <string>

Probabilities make_uniform_probabilities(const Grammar& grammar);

// Generate a string from the grammar. Respects max_depth and max_total_length.
std::string generate(
    const Grammar& grammar,
    const std::string& symbol,
    const Probabilities& probs,
    std::mt19937& rng,
    int depth = 0,
    int max_depth = 20,
    std::size_t max_total_length = 256,
    std::size_t* current_length = nullptr
);
