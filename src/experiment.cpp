#include "experiment.h"
#include <utility>

Grammar build_rich_grammar() {
    Grammar g;
    g.start_symbol = "<S>";

    // Start symbol
    g.productions["<S>"] = {
        {"<E>"}
    };

    // Expression with ternary, assignment-level complexity
    g.productions["<E>"] = {
        {"<T>"},
        {"<T>", "+", "<E>"},
        {"<T>", "-", "<E>"},
        {"<T>", "?", "<E>", ":", "<E>"}  // ternary operator
    };

    // Term with comparison and logical operators
    g.productions["<T>"] = {
        {"<R>"},
        {"<R>", "*", "<T>"},
        {"<R>", "/", "<T>"},
        {"<R>", "%", "<T>"},
        {"<R>", "<", "<T>"},
        {"<R>", ">", "<T>"},
        {"<R>", "==", "<T>"},
        {"<R>", "!=", "<T>"},
        {"<R>", "&&", "<T>"},
        {"<R>", "||", "<T>"}
    };

    // Relational/Additive (used to create depth)
    g.productions["<R>"] = {
        {"<F>"},
        {"<U>", "<R>"}  // unary operators
    };

    // Factor: primary expressions
    g.productions["<F>"] = {
        {"<NUM>"},
        {"(", "<E>", ")"},
        {"<FUNC>"},
        {"<VAR>"}
    };

    // Unary operators
    g.productions["<U>"] = {
        {"+"},
        {"-"},
        {"!"},
        {"~"}
    };

    // Functions
    g.productions["<FUNC>"] = {
        {"sin(", "<E>", ")"},
        {"cos(", "<E>", ")"},
        {"abs(", "<E>", ")"},
        {"sqrt(", "<E>", ")"}
    };

    // Variables
    g.productions["<VAR>"] = {
        {"x"},
        {"y"},
        {"z"}
    };

    // Numbers: integer, float, hex, scientific
    g.productions["<NUM>"] = {
        {"<INT>"},
        {"<FLOAT>"},
        {"<HEX>"},
        {"<SCI>"}
    };

    // Integer
    g.productions["<INT>"] = {
        {"<DIGIT>"},
        {"<DIGIT>", "<INT>"}
    };

    // Float: digits.digits
    g.productions["<FLOAT>"] = {
        {"<INT>", ".", "<INT>"}
    };

    // Hex: 0xhexdigits
    g.productions["<HEX>"] = {
        {"0x", "<HEXDIG>"},
        {"0x", "<HEXDIG>", "<HEXDIG>"},
        {"0x", "<HEXDIG>", "<HEXDIG>", "<HEXDIG>"}
    };

    // Scientific: digits e digits
    g.productions["<SCI>"] = {
        {"<INT>", "e", "<INT>"},
        {"<INT>", "e", "+", "<INT>"},
        {"<INT>", "e", "-", "<INT>"}
    };

    // Digits
    g.productions["<DIGIT>"] = {
        {"0"}, {"1"}, {"2"}, {"3"}, {"4"},
        {"5"}, {"6"}, {"7"}, {"8"}, {"9"}
    };

    // Hex digits
    g.productions["<HEXDIG>"] = {
        {"0"}, {"1"}, {"2"}, {"3"}, {"4"},
        {"5"}, {"6"}, {"7"}, {"8"}, {"9"},
        {"a"}, {"b"}, {"c"}, {"d"}, {"e"}, {"f"}
    };

    return g;
}

void run_experiments(
    const Grammar& grammar,
    const GAConfig& cfg,
    int runs,
    std::vector<RunSummary>& baseline_runs,
    std::vector<RunSummary>& adaptive_runs,
    std::vector<GenerationRecord>& generation_logs
) {
    baseline_runs.clear();
    adaptive_runs.clear();
    generation_logs.clear();

    baseline_runs.reserve(runs);
    adaptive_runs.reserve(runs);

    for (int run = 0; run < runs; ++run) {
        GAConfig local_cfg = cfg;
        local_cfg.seed = cfg.seed + static_cast<std::uint32_t>(run);

        baseline_runs.push_back(run_baseline_experiment(grammar, local_cfg, run));

        AdaptiveResult adaptive = run_adaptive_experiment(grammar, local_cfg, run);
        adaptive_runs.push_back(adaptive.summary);

        for (const auto& rec : adaptive.generation_log) {
            generation_logs.push_back(rec);
        }
    }
}
