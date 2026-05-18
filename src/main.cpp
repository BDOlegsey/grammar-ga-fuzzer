// main.cpp — CLI entry point and experiment routing
#include "ablation.h"
#include "cli_options.h"
#include "experiment.h"
#include "logger.h"
#include "report.h"
#include "sweep.h"
#include <filesystem>
#include <iostream>
#include <vector>

static GAConfig config_from_options(const CLIOptions& opt) {
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
    return cfg;
}

static void print_banner(const CLIOptions& opt, const ISut& sut) {
    std::cout << "GrammarFuzzer GA\n";
    std::cout << "================\n";
    std::cout << "SUT:        " << sut.name() << " (" << sut.branch_count() << " branches)\n";
    std::cout << "Mode:       " << opt.mode << "\n";
    std::cout << "Runs:       " << opt.runs << "\n";
    std::cout << "Population: " << opt.population_size << "\n";
    std::cout << "Generations: " << opt.generations << "\n";
    std::cout << "Samples/individual: " << opt.samples_per_individual << "\n";
    std::cout << "Fitness weights: alpha=" << opt.alpha
              << " beta=" << opt.beta
              << " gamma=" << opt.gamma
              << " delta=" << opt.delta << "\n";
    std::cout << "Max depth: " << opt.max_depth << "\n";
    std::cout << "Max input length: " << opt.max_total_length << "\n";
    std::cout << "Seed: " << opt.seed << "\n\n";
}

static std::vector<std::string> resolve_sut_list(const std::string& sut_name) {
    if (sut_name == "all") {
        return {"json", "csv", "url"};
    }
    return {sut_name};
}

int main(int argc, char** argv) {
    try {
        CLIOptions opt = parse_options(argc, argv);
        std::filesystem::create_directories("results");

        if (opt.mode == "sweep") {
            SweepGrid grid;
            grid.runs_per_config = opt.runs;
            grid.base_seed = opt.seed;
            auto sut_list = resolve_sut_list(opt.sut_name);
            if (opt.sut_name == "all") {
                sut_list = {"arithmetic", "json", "csv", "url"};
            } else if (opt.sut_name != "all") {
                sut_list = {opt.sut_name};
            }
            auto sut = make_sut(sut_list.front());
            print_banner(opt, *sut);
            run_sweep(sut_list, grid, opt);
            return 0;
        }

        if (opt.mode == "ablation") {
            auto sut = make_sut(opt.sut_name);
            print_banner(opt, *sut);
            run_ablation(*sut, opt);
            return 0;
        }

        auto sut_names = resolve_sut_list(opt.sut_name);
        for (const auto& name : sut_names) {
            auto sut = make_sut(name);
            Grammar grammar = grammar_for_sut(name);
            GAConfig cfg = config_from_options(opt);

            print_banner(opt, *sut);

            std::vector<RunSummary> baseline_runs;
            std::vector<RunSummary> adaptive_runs;
            std::vector<GenerationRecord> generation_logs;

            run_experiments(grammar, cfg, *sut, opt.runs,
                            baseline_runs, adaptive_runs, generation_logs);

            const std::string suffix = (opt.sut_name == "all") ? "_" + name : "";
            RunCsvLogger run_logger("results/runs" + suffix + ".csv");
            GenerationCsvLogger gen_logger("results/generations" + suffix + ".csv");
            RuleWeightCsvLogger weight_logger("results/rule_weights_" + name + ".csv");

            for (const auto& r : baseline_runs) run_logger.append(r);
            for (const auto& r : adaptive_runs) run_logger.append(r);
            for (const auto& g : generation_logs) gen_logger.append(g);

            if (opt.log_weights) {
                for (const auto& g : generation_logs) {
                    if (g.has_weights) {
                        weight_logger.append(g.run_id, g.generation, g);
                    }
                }
            }

            std::cout << "Completed " << opt.runs << " baseline/adaptive runs for " << name << ".\n";
            print_group_summary("BASELINE", baseline_runs);
            print_group_summary("ADAPTIVE", adaptive_runs);
            print_comparison(name, baseline_runs, adaptive_runs);

            std::cout << "\nCSV written to:\n";
            std::cout << "  results/runs" << suffix << ".csv\n";
            std::cout << "  results/generations" << suffix << ".csv\n";
            if (opt.log_weights) {
                std::cout << "  results/rule_weights_" << name << ".csv\n";
            }
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
