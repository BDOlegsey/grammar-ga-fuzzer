#include "generator.h"
#include "utils.h"
#include <algorithm>
#include <random>

static bool is_nonterminal(const Grammar& grammar, const std::string& token) {
    return grammar.productions.find(token) != grammar.productions.end();
}

static bool rule_is_terminal_only(const Grammar& grammar, const Rule& rule) {
    for (const auto& token : rule) {
        if (is_nonterminal(grammar, token)) {
            return false;
        }
    }
    return true;
}

static std::size_t rule_terminal_count(const Grammar& grammar, const Rule& rule) {
    std::size_t count = 0;
    for (const auto& token : rule) {
        if (!is_nonterminal(grammar, token)) count++;
    }
    return count;
}

static std::size_t rule_nonterminal_count(const Grammar& grammar, const Rule& rule) {
    std::size_t count = 0;
    for (const auto& token : rule) {
        if (is_nonterminal(grammar, token)) count++;
    }
    return count;
}

Probabilities make_uniform_probabilities(const Grammar& grammar) {
    Probabilities probs;
    for (const auto& [lhs, rules] : grammar.productions) {
        probs[lhs] = normalize(std::vector<double>(rules.size(), 1.0));
    }
    return probs;
}

static std::size_t choose_terminal_friendly_rule(
    const Grammar& grammar,
    const std::vector<Rule>& rules,
    const std::vector<double>& weights,
    std::mt19937& rng
) {
    // Prefer rules with fewer nonterminals and more terminals
    std::vector<std::size_t> candidates;
    for (std::size_t i = 0; i < rules.size(); ++i) {
        if (rule_is_terminal_only(grammar, rules[i])) {
            candidates.push_back(i);
        }
    }

    if (!candidates.empty()) {
        std::vector<double> cweights;
        cweights.reserve(candidates.size());
        for (std::size_t idx : candidates) {
            cweights.push_back(weights[idx]);
        }
        std::discrete_distribution<std::size_t> dist(cweights.begin(), cweights.end());
        return candidates[dist(rng)];
    }

    // Pick rule with minimum nonterminals, breaking ties by max terminals
    std::size_t best_idx = 0;
    std::size_t best_nt = static_cast<std::size_t>(-1);
    std::size_t best_term = 0;

    for (std::size_t i = 0; i < rules.size(); ++i) {
        std::size_t nt = rule_nonterminal_count(grammar, rules[i]);
        std::size_t t = rule_terminal_count(grammar, rules[i]);
        if (nt < best_nt || (nt == best_nt && t > best_term)) {
            best_nt = nt;
            best_term = t;
            best_idx = i;
        }
    }
    return best_idx;
}

std::string generate(
    const Grammar& grammar,
    const std::string& symbol,
    const Probabilities& probs,
    std::mt19937& rng,
    int depth,
    int max_depth,
    std::size_t max_total_length,
    std::size_t* current_length
) {
    // Track length locally if caller doesn't provide
    std::size_t local_len = 0;
    bool local_track = (current_length == nullptr);
    if (local_track) {
        current_length = &local_len;
    }

    // Hard stop if we've exceeded length budget
    if (*current_length >= max_total_length) {
        return "";
    }

    auto it = grammar.productions.find(symbol);
    if (it == grammar.productions.end()) {
        // Terminal symbol
        if (*current_length + symbol.size() > max_total_length) {
            // Truncate terminal to fit
            std::size_t remaining = max_total_length - *current_length;
            *current_length += remaining;
            return symbol.substr(0, remaining);
        }
        *current_length += symbol.size();
        return symbol;
    }

    const auto& rules = it->second;
    auto pit = probs.find(symbol);

    std::vector<double> weights;
    if (pit != probs.end() && pit->second.size() == rules.size()) {
        weights = pit->second;
    } else {
        weights.assign(rules.size(), 1.0);
        weights = normalize(weights);
    }

    std::size_t chosen = 0;
    if (depth >= max_depth) {
        chosen = choose_terminal_friendly_rule(grammar, rules, weights, rng);
    } else {
        std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
        chosen = dist(rng);
    }

    std::string out;
    for (const auto& token : rules[chosen]) {
        out += generate(grammar, token, probs, rng, depth + 1, max_depth,
                        max_total_length, current_length);
        if (*current_length >= max_total_length) {
            break;
        }
    }
    return out;
}
