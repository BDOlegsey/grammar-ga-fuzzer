// cli_options.cpp — command-line option parsing
#include "cli_options.h"
#include <sstream>
#include <stdexcept>

static std::string get_arg(int argc, char** argv, const std::string& key, const std::string& def) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) return argv[i + 1];
    }
    return def;
}

static bool has_flag(int argc, char** argv, const std::string& key) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == key) return true;
    }
    return false;
}

static std::vector<int> parse_int_list(const std::string& s) {
    std::vector<int> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) out.push_back(std::stoi(item));
    }
    return out;
}

static std::vector<double> parse_double_list(const std::string& s) {
    std::vector<double> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) out.push_back(std::stod(item));
    }
    return out;
}

CLIOptions parse_options(int argc, char** argv) {
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
    opt.max_total_length = static_cast<std::size_t>(
        std::stoul(get_arg(argc, argv, "--maxlen", std::to_string(opt.max_total_length))));
    opt.sut_name = get_arg(argc, argv, "--sut", opt.sut_name);
    opt.mode = get_arg(argc, argv, "--mode", opt.mode);
    opt.sweep_pop = parse_int_list(get_arg(argc, argv, "--sweep-pop", "20,40,80"));
    opt.sweep_mut = parse_double_list(get_arg(argc, argv, "--sweep-mut", "0.05,0.12,0.25"));
    opt.log_weights = !has_flag(argc, argv, "--no-weights");
    return opt;
}
