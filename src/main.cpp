#include "experiment.h"
#include "logger.h"
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
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
};

static std::string get_arg(int argc, char** argv, const std::string& key, const std::string& def) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) return argv[i + 1];
    }
    return def;
}

static CLIOptions parse_options(int argc, char** argv) {
    CLIOptions opt;
    opt.runs = std::stoi(get_arg(argc, argv, "--runs", std::to_string(opt.runs)));
    opt.population_size = std::stoi(get_arg(argc, argv, "--pop", std::to_string(opt.population_size)));
    opt.generations = std::stoi(get_arg(argc, argv, "--gen", std::to_string(opt.generations)));
    opt.samples_per_individual = std::stoi(get_arg(argc, argv, "--samples", std::to_string(opt.samples_per_individual)));
    opt.elite_count = std::stoi(get_arg(argc, argv, "--elite", std::to_string(opt.elite_count)));
    opt.tournament_size = std::stoi(get_arg(argc, argv, "--tour", std::to_string(opt.tournament_size)));
    opt.crossover_rate = std::stod(get_arg(argc, argv, "--cross", std::to_string(opt.crossover_rate)));
    opt.mutation_rate = std::stod(get_arg(argc, argv, "--mut", std::to_string(opt.mutation_rate)));
    opt.alpha = std::stod(get_arg(argc, argv, "--alpha", std::to_string(opt.alpha)));
    opt.beta = std::stod(get_arg(argc, argv, "--beta", std::to_string(opt.beta)));
    opt.gamma = std::stod(get_arg(argc, argv, "--gamma", std::to_string(opt.gamma)));
    opt.delta = std::stod(get_arg(argc, argv, "--delta", std::to_string(opt.delta)));
    opt.max_depth = std::stoi(get_arg(argc, argv, "--depth", std::to_string(opt.max_depth)));
    opt.seed = static_cast<std::uint32_t>(std::stoul(get_arg(argc, argv, "--seed", std::to_string(opt.seed))));
    opt.max_total_length = static_cast<std::size_t>(std::stoul(get_arg(argc, argv, "--maxlen", std::to_string(opt.max_total_length))));
    return opt;
}

// ---- Statistics helpers ----

template <typename T, typename Getter>
static std::pair<double, double> mean_std(const std::vector<T>& data, Getter getter) {
    if (data.empty()) return {0.0, 0.0};
    double sum = 0.0;
    for (const auto& x : data) sum += getter(x);
    double mean = sum / static_cast<double>(data.size());
    double var = 0.0;
    for (const auto& x : data) {
        double d = getter(x) - mean;
        var += d * d;
    }
    var /= static_cast<double>(data.size());
    return {mean, std::sqrt(var)};
}

template <typename T, typename Getter>
static double median(const std::vector<T>& data, Getter getter) {
    if (data.empty()) return 0.0;
    std::vector<double> vals;
    vals.reserve(data.size());
    for (const auto& x : data) vals.push_back(getter(x));
    std::sort(vals.begin(), vals.end());
    std::size_t n = vals.size();
    if (n % 2 == 1) return vals[n / 2];
    return (vals[n / 2 - 1] + vals[n / 2]) * 0.5;
}

template <typename T, typename Getter>
static std::pair<double, double> iqr(const std::vector<T>& data, Getter getter) {
    if (data.empty()) return {0.0, 0.0};
    std::vector<double> vals;
    vals.reserve(data.size());
    for (const auto& x : data) vals.push_back(getter(x));
    std::sort(vals.begin(), vals.end());
    std::size_t n = vals.size();
    auto percentile = [&](double p) -> double {
        double pos = p * (n - 1);
        std::size_t lo = static_cast<std::size_t>(pos);
        std::size_t hi = std::min(lo + 1, n - 1);
        double frac = pos - lo;
        return vals[lo] * (1.0 - frac) + vals[hi] * frac;
    };
    return {percentile(0.25), percentile(0.75)};
}

template <typename T, typename Getter>
static double min_val(const std::vector<T>& data, Getter getter) {
    if (data.empty()) return 0.0;
    double m = getter(data[0]);
    for (const auto& x : data) m = std::min(m, getter(x));
    return m;
}

template <typename T, typename Getter>
static double max_val(const std::vector<T>& data, Getter getter) {
    if (data.empty()) return 0.0;
    double m = getter(data[0]);
    for (const auto& x : data) m = std::max(m, getter(x));
    return m;
}

// ---- Summary printer ----

static void print_group_summary(const std::string& title, const std::vector<RunSummary>& runs) {
    auto [cov_mean, cov_std] = mean_std(runs, [](const RunSummary& r) { return r.coverage; });
    auto [bug_mean, bug_std] = mean_std(runs, [](const RunSummary& r) { return static_cast<double>(r.unique_bugs); });
    auto [time_mean, time_std] = mean_std(runs, [](const RunSummary& r) { return r.runtime_sec; });
    auto [fit_mean, fit_std] = mean_std(runs, [](const RunSummary& r) { return r.best_fitness; });
    auto [nov_mean, nov_std] = mean_std(runs, [](const RunSummary& r) { return r.novelty_score; });
    auto [div_mean, div_std] = mean_std(runs, [](const RunSummary& r) { return r.diversity_score; });

    double cov_med = median(runs, [](const RunSummary& r) { return r.coverage; });
    double bug_med = median(runs, [](const RunSummary& r) { return static_cast<double>(r.unique_bugs); });
    double time_med = median(runs, [](const RunSummary& r) { return r.runtime_sec; });
    double fit_med = median(runs, [](const RunSummary& r) { return r.best_fitness; });

    auto [cov_q1, cov_q3] = iqr(runs, [](const RunSummary& r) { return r.coverage; });
    auto [bug_q1, bug_q3] = iqr(runs, [](const RunSummary& r) { return static_cast<double>(r.unique_bugs); });

    std::cout << "\n[" << title << "]\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "coverage:       " << cov_mean << " +/- " << cov_std
              << "  [median " << cov_med << ", IQR " << cov_q1 << "-" << cov_q3 << "]\n";
    std::cout << "bugs:           " << bug_mean << " +/- " << bug_std
              << "  [median " << bug_med << ", IQR " << bug_q1 << "-" << bug_q3 << "]\n";
    std::cout << "runtime s:      " << time_mean << " +/- " << time_std
              << "  [median " << time_med << "]\n";
    std::cout << "fitness:        " << fit_mean << " +/- " << fit_std
              << "  [median " << fit_med << "]\n";
    std::cout << "novelty:        " << nov_mean << " +/- " << nov_std << "\n";
    std::cout << "diversity:      " << div_mean << " +/- " << div_std << "\n";
    std::cout << "avg_input_len:  " << mean_std(runs, [](const RunSummary& r) { return r.avg_input_length; }).first << "\n";
}

// ---- Mann-Whitney U test (approximate) ----

template <typename T, typename Getter>
static double mann_whitney_u(const std::vector<T>& a, const std::vector<T>& b, Getter getter) {
    // Combine and rank
    std::vector<std::pair<double, int>> combined;
    combined.reserve(a.size() + b.size());
    for (const auto& x : a) combined.push_back({getter(x), 0});
    for (const auto& x : b) combined.push_back({getter(x), 1});
    std::sort(combined.begin(), combined.end(),
              [](const auto& p1, const auto& p2) { return p1.first < p2.first; });

    double rank_sum_a = 0.0;
    std::size_t n = combined.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (combined[i].second == 0) rank_sum_a += static_cast<double>(i + 1);
    }

    double n1 = static_cast<double>(a.size());
    double n2 = static_cast<double>(b.size());
    double U1 = rank_sum_a - n1 * (n1 + 1.0) / 2.0;
    double U2 = n1 * n2 - U1;
    return std::min(U1, U2);
}

// ---- Main ----

int main(int argc, char** argv) {
    try {
        CLIOptions opt = parse_options(argc, argv);

        GAConfig cfg;
        cfg.population_size = opt.population_size;
        cfg.generations = opt.generations;
        cfg.samples_per_individual = opt.samples_per_individual;
        cfg.elite_count = opt.elite_count;
        cfg.tournament_size = opt.tournament_size;
        cfg.crossover_rate = opt.crossover_rate;
        cfg.mutation_rate = opt.mutation_rate;
        cfg.alpha = opt.alpha;
        cfg.beta = opt.beta;
        cfg.gamma = opt.gamma;
        cfg.delta = opt.delta;
        cfg.max_depth = opt.max_depth;
        cfg.seed = opt.seed;
        cfg.max_total_length = opt.max_total_length;

        std::filesystem::create_directories("results");

        Grammar grammar = build_rich_grammar();

        std::cout << "Adaptive Grammar Fuzzer\n";
        std::cout << "=======================\n";
        std::cout << "Runs: " << opt.runs << "\n";
        std::cout << "Population: " << cfg.population_size << "\n";
        std::cout << "Generations: " << cfg.generations << "\n";
        std::cout << "Samples/individual: " << cfg.samples_per_individual << "\n";
        std::cout << "Fitness weights: alpha=" << cfg.alpha
                  << " beta=" << cfg.beta
                  << " gamma=" << cfg.gamma
                  << " delta=" << cfg.delta << "\n";
        std::cout << "Max depth: " << cfg.max_depth << "\n";
        std::cout << "Max input length: " << cfg.max_total_length << "\n";
        std::cout << "Seed: " << cfg.seed << "\n\n";

        std::vector<RunSummary> baseline_runs;
        std::vector<RunSummary> adaptive_runs;
        std::vector<GenerationRecord> generation_logs;

        run_experiments(grammar, cfg, opt.runs, baseline_runs, adaptive_runs, generation_logs);

        RunCsvLogger run_logger("results/runs.csv");
        GenerationCsvLogger gen_logger("results/generations.csv");

        for (const auto& r : baseline_runs) run_logger.append(r);
        for (const auto& r : adaptive_runs) run_logger.append(r);
        for (const auto& g : generation_logs) gen_logger.append(g);

        std::cout << "Completed " << opt.runs << " baseline/adaptive runs.\n";
        print_group_summary("BASELINE", baseline_runs);
        print_group_summary("ADAPTIVE", adaptive_runs);

        // Statistical comparison
        std::cout << "\n[STATISTICAL COMPARISON]\n";
        auto [bcm, _1] = mean_std(baseline_runs, [](const RunSummary& r) { return r.coverage; });
        auto [acm, _2] = mean_std(adaptive_runs, [](const RunSummary& r) { return r.coverage; });
        auto [bbm, _3] = mean_std(baseline_runs, [](const RunSummary& r) { return static_cast<double>(r.unique_bugs); });
        auto [abm, _4] = mean_std(adaptive_runs, [](const RunSummary& r) { return static_cast<double>(r.unique_bugs); });

        std::cout << "Coverage improvement: " << std::fixed << std::setprecision(4)
                  << ((acm - bcm) / bcm * 100.0) << "%\n";
        std::cout << "Bug improvement:      " << std::fixed << std::setprecision(4)
                  << ((abm - bbm) / std::max(bbm, 1.0) * 100.0) << "%\n";

        double cov_u = mann_whitney_u(baseline_runs, adaptive_runs,
                                      [](const RunSummary& r) { return r.coverage; });
        std::cout << "Mann-Whitney U (coverage): " << std::fixed << std::setprecision(2)
                  << cov_u << "\n";

        std::cout << "\nCSV written to:\n";
        std::cout << "  results/runs.csv\n";
        std::cout << "  results/generations.csv\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
