// sweep.cpp — hyperparameter grid search with Welch t-test reporting
#include "sweep.h"
#include "experiment.h"
#include "logger.h"
#include "stats.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

void run_sweep(const std::vector<std::string>& sut_names,
               const SweepGrid& /*grid*/,
               const CLIOptions& opt) {
    std::filesystem::create_directories("results");

    for (const auto& sut_name : sut_names) {
        auto sut = make_sut(sut_name);
        Grammar grammar = grammar_for_sut(sut_name);
        SweepCsvLogger logger("results/sweep_" + sut_name + ".csv");

        SweepRecord best_rec;
        double best_fitness = -1.0;
        std::vector<double> best_covs;
        std::vector<double> baseline_covs;

        for (int pop : opt.sweep_pop) {
            for (double mut : opt.sweep_mut) {
                std::vector<double> covs, bugs, fits, times;
                GAConfig cfg;
                cfg.population_size = pop;
                cfg.generations = opt.generations;
                cfg.samples_per_individual = opt.samples_per_individual;
                cfg.elite_count = opt.elite_count;
                cfg.tournament_size = opt.tournament_size;
                cfg.crossover_rate = opt.crossover_rate;
                cfg.mutation_rate = mut;
                cfg.alpha = opt.alpha;
                const double rest = 1.0 - cfg.alpha;
                cfg.beta = rest * 5.0 / 9.0;
                cfg.gamma = rest * 2.0 / 9.0;
                cfg.delta = rest * 2.0 / 9.0;
                cfg.max_depth = opt.max_depth;
                cfg.seed = opt.seed;
                cfg.max_total_length = opt.max_total_length;

                for (int run = 0; run < opt.runs; ++run) {
                    GAConfig local = cfg;
                    local.seed = opt.seed + static_cast<std::uint32_t>(run);
                    AdaptiveResult ar = run_adaptive_experiment(grammar, local, *sut, run);
                    covs.push_back(ar.summary.coverage);
                    bugs.push_back(static_cast<double>(ar.summary.unique_bugs));
                    fits.push_back(ar.summary.best_fitness);
                    times.push_back(ar.summary.runtime_sec);
                }

                auto [cm, cs] = mean_and_std(covs);
                auto [bm, bs] = mean_and_std(bugs);
                auto [fm, fs] = mean_and_std(fits);
                auto [tm, _] = mean_and_std(times);

                SweepRecord rec;
                rec.sut_name = sut_name;
                rec.population_size = pop;
                rec.generations = opt.generations;
                rec.mutation_rate = mut;
                rec.crossover_rate = opt.crossover_rate;
                rec.alpha = opt.alpha;
                rec.seed = opt.seed;
                rec.coverage_mean = cm;
                rec.coverage_std = cs;
                rec.bugs_mean = bm;
                rec.bugs_std = bs;
                rec.fitness_mean = fm;
                rec.fitness_std = fs;
                rec.runtime_mean = tm;
                logger.append(rec);

                if (fm > best_fitness) {
                    best_fitness = fm;
                    best_rec = rec;
                    best_covs = covs;
                }
            }
        }

        GAConfig base_cfg;
        base_cfg.population_size = opt.population_size;
        base_cfg.generations = opt.generations;
        base_cfg.samples_per_individual = opt.samples_per_individual;
        base_cfg.seed = opt.seed;
        base_cfg.max_depth = opt.max_depth;
        base_cfg.max_total_length = opt.max_total_length;
        for (int run = 0; run < opt.runs; ++run) {
            GAConfig local = base_cfg;
            local.seed = opt.seed + static_cast<std::uint32_t>(run);
            RunSummary br = run_baseline_experiment(grammar, local, *sut, run);
            baseline_covs.push_back(br.coverage);
        }

        double t = welch_t_stat(best_covs, baseline_covs);
        double p = welch_p_value(best_covs, baseline_covs);
        double d = cohens_d(best_covs, baseline_covs);

        std::cout << "Best config for [" << sut_name << "]:\n"
                  << "  population_size=" << best_rec.population_size
                  << "  mutation_rate=" << best_rec.mutation_rate
                  << "  crossover_rate=" << best_rec.crossover_rate
                  << "  alpha=" << std::fixed << std::setprecision(2) << best_rec.alpha
                  << "  coverage_mean=" << std::setprecision(4) << best_rec.coverage_mean
                  << "  bugs_mean=" << best_rec.bugs_mean << "\n"
                  << "  Welch t vs baseline: p=" << p
                  << "  effect_size=" << d << "  t=" << t << "\n";

        std::ofstream best_out("results/best_config.txt", std::ios::app);
        best_out << "Best config for [" << sut_name << "]:\n"
                 << "  population_size=" << best_rec.population_size
                 << "  mutation_rate=" << best_rec.mutation_rate
                 << "  crossover_rate=" << best_rec.crossover_rate
                 << "  alpha=" << best_rec.alpha
                 << "  coverage_mean=" << best_rec.coverage_mean
                 << "  bugs_mean=" << best_rec.bugs_mean
                 << "  Welch t vs baseline: p=" << p
                 << "  effect_size=" << d << "\n";
    }
}
