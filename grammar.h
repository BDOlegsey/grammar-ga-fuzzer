#pragma once

#include <string>
#include <unordered_map>
#include <vector>

using Rule = std::vector<std::string>;
using Probabilities = std::unordered_map<std::string, std::vector<double>>;

struct Grammar {
    std::string start_symbol{"<S>"};
    std::unordered_map<std::string, std::vector<Rule>> productions;
};
