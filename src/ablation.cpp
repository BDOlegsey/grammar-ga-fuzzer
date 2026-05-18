// ablation.cpp — ablation study over fitness weight configurations
#include "ablation.h"
#include "experiment.h"
#include "logger.h"
#include "stats.h"
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

struct AblationCondition {
    std::string name;
    double alpha;
    double beta;
    double gamma;
    double delta;
};

} // namespace

void run_ablation(const ISut& sut, const CLIOptions& opt) {
    const std::string sut_name = sut.name();
    std::filesystem::create_directories("results");

    AblationCsvLogger run_logger("results/ablation_" + sut_name + ".csv");
    AblationGenerationCsvLogger gen_logger("results/ablation_generations_" + sut_name + ".csv");

    const std::vector<AblationCondition> conditions = {
        {"COV_ONLY", 1.0, 0.0, 0.0, 0.0},
        {"BUGS_ONLY", 0.0, 1.0, 0.0, 0.0},
        {"COV_BUGS", 0.5, 0.5, 0.0, 0.0},
        {"FULL", 0.35, 0.35, 0.15, 0.15},
    };

    Grammar grammar = grammar_for_sut(sut_name);

    for (const auto& cond : conditions) {
        for (int run = 0; run < opt.runs; ++run) {
            GAConfig cfg;
            cfg.population_size = opt.population_size;
            cfg.generations = opt.generations;
            cfg.samples_per_individual = opt.samples_per_individual;
            cfg.elite_count = opt.elite_count;
            cfg.tournament_size = opt.tournament_size;
            cfg.crossover_rate = opt.crossover_rate;
            cfg.mutation_rate = opt.mutation_rate;
            cfg.alpha = cond.alpha;
            cfg.beta = cond.beta;
            cfg.gamma = cond.gamma;
            cfg.delta = cond.delta;
            cfg.max_depth = opt.max_depth;
            cfg.seed = opt.seed + static_cast<std::uint32_t>(run);
            cfg.max_total_length = opt.max_total_length;

            AdaptiveResult ar = run_adaptive_experiment(grammar, cfg, sut, run);

            AblationRunRecord rec;
            rec.condition = cond.name;
            rec.seed = cfg.seed;
            rec.coverage = ar.summary.coverage;
            rec.unique_bugs = ar.summary.unique_bugs;
            rec.best_fitness = ar.summary.best_fitness;
            rec.novelty_score = ar.summary.novelty_score;
            rec.diversity_score = ar.summary.diversity_score;
            rec.runtime_sec = ar.summary.runtime_sec;
            run_logger.append(rec);

            for (auto& grec : ar.generation_log) {
                grec.run_id = run;
                gen_logger.append(cond.name, grec);
            }
        }
    }

    std::cout << "Ablation complete for " << sut_name
              << " -> results/ablation_" << sut_name << ".csv\n";
}
