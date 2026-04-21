#include "ga.h"
#include "generator.h"
#include "metrics.h"
#include "sut.h"
#include "utils.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>

namespace {

Candidate evaluate_candidate(
    const Grammar& grammar,
    const GAConfig& cfg,
    const Individual& ind,
    std::mt19937& rng,
    CorpusStats& overall_stats,
    std::array<bool, kCoverageBranches>& globally_covered,
    std::array<std::size_t, kCoverageBranches>& global_hit_counts
) {
    MiniSUT sut;
    CorpusStats local_stats;
    std::array<std::size_t, kCoverageBranches> local_hit_counts{};

    for (int i = 0; i < cfg.samples_per_individual; ++i) {
        std::string input = generate(grammar, grammar.start_symbol, ind, rng,
                                     0, cfg.max_depth, cfg.max_total_length);
        ExecutionResult exec = sut.run(input);

        // Track per-branch hit counts for diversity
        for (std::size_t b = 0; b < kCoverageBranches; ++b) {
            if (exec.branch_hits[b]) {
                local_hit_counts[b]++;
                global_hit_counts[b]++;
            }
        }

        absorb(local_stats, exec, &globally_covered);
        absorb(overall_stats, exec, nullptr);
    }

    double cov = coverage_ratio(local_stats);
    double bugs = normalized_bug_score(local_stats, 10); // cap at 10
    double nov = static_cast<double>(local_stats.novelty_hits)
               / static_cast<double>(kCoverageBranches);
    double div = diversity_score(local_hit_counts);

    Candidate cand;
    cand.ind = ind;
    cand.stats = local_stats;
    cand.branch_hits = local_hit_counts;
    cand.fitness = cfg.alpha * cov
                 + cfg.beta  * bugs
                 + cfg.gamma * nov
                 + cfg.delta * div;
    return cand;
}

double average_fitness(const std::vector<Candidate>& pop) {
    if (pop.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& c : pop) sum += c.fitness;
    return sum / static_cast<double>(pop.size());
}

double population_diversity(const std::vector<Candidate>& pop) {
    if (pop.size() < 2) return 0.0;
    std::array<std::size_t, kCoverageBranches> total_counts{};
    for (const auto& c : pop) {
        for (std::size_t i = 0; i < kCoverageBranches; ++i) {
            total_counts[i] += c.branch_hits[i];
        }
    }
    return diversity_score(total_counts);
}

} // namespace

Individual random_individual(const Grammar& grammar, std::mt19937& rng) {
    std::uniform_real_distribution<double> dist(0.2, 1.0);
    Individual ind;
    for (const auto& [nt, rules] : grammar.productions) {
        std::vector<double> raw;
        raw.reserve(rules.size());
        for (std::size_t i = 0; i < rules.size(); ++i) {
            raw.push_back(dist(rng));
        }
        ind[nt] = normalize(raw);
    }
    return ind;
}

Individual tournament_select(const std::vector<Candidate>& population, std::mt19937& rng, int tournament_size) {
    std::uniform_int_distribution<std::size_t> pick(0, population.size() - 1);

    std::size_t best = pick(rng);
    for (int i = 1; i < tournament_size; ++i) {
        std::size_t idx = pick(rng);
        if (population[idx].fitness > population[best].fitness) {
            best = idx;
        }
    }
    return population[best].ind;
}

Individual crossover(const Individual& p1, const Individual& p2, std::mt19937& rng, double crossover_rate) {
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    std::uniform_int_distribution<int> choose_parent(0, 1);

    if (coin(rng) > crossover_rate) {
        return (choose_parent(rng) == 0) ? p1 : p2;
    }

    // Blend crossover (BLX-alpha) with adaptive mixing
    Individual child;
    for (const auto& [nt, vec1] : p1) {
        const auto& vec2 = p2.at(nt);
        std::vector<double> mixed;
        mixed.reserve(vec1.size());

        for (std::size_t i = 0; i < vec1.size(); ++i) {
            double alpha_blx = 0.3;
            double min_v = std::min(vec1[i], vec2[i]);
            double max_v = std::max(vec1[i], vec2[i]);
            double range = max_v - min_v;
            std::uniform_real_distribution<double> blend(
                min_v - alpha_blx * range,
                max_v + alpha_blx * range
            );
            mixed.push_back(clamp01(blend(rng)));
        }
        child[nt] = normalize(mixed);
    }
    return child;
}

void mutate(Individual& ind, std::mt19937& rng, double mutation_rate) {
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    // Use Cauchy distribution for occasional large jumps (exploration)
    std::cauchy_distribution<double> heavy_noise(0.0, 0.08);
    std::normal_distribution<double> light_noise(0.0, 0.05);

    for (auto& [nt, vec] : ind) {
        bool changed = false;
        for (double& p : vec) {
            if (coin(rng) < mutation_rate) {
                // 20% chance of heavy mutation, 80% light
                if (coin(rng) < 0.2) {
                    p += heavy_noise(rng);
                } else {
                    p += light_noise(rng);
                }
                p = clamp01(p);
                // Ensure we don't get exact zeros
                if (p < 1e-9) p = 1e-9;
                changed = true;
            }
        }
        if (changed) {
            vec = normalize(vec);
        }
    }
}

RunSummary run_baseline_experiment(const Grammar& grammar, const GAConfig& cfg, int run_id) {
    auto start = std::chrono::high_resolution_clock::now();
    std::mt19937 rng(cfg.seed + static_cast<std::uint32_t>(run_id));

    CorpusStats overall;
    Individual uniform = make_uniform_probabilities(grammar);
    std::array<bool, kCoverageBranches> globally_covered{};

    const std::size_t budget = static_cast<std::size_t>(cfg.population_size)
                             * static_cast<std::size_t>(cfg.generations)
                             * static_cast<std::size_t>(cfg.samples_per_individual);

    for (std::size_t i = 0; i < budget; ++i) {
        std::string input = generate(grammar, grammar.start_symbol, uniform, rng,
                                     0, cfg.max_depth, cfg.max_total_length);
        ExecutionResult exec = MiniSUT().run(input);
        absorb(overall, exec, &globally_covered);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    RunSummary summary;
    summary.mode = "baseline";
    summary.seed = cfg.seed + static_cast<std::uint32_t>(run_id);
    summary.total_inputs = overall.total_inputs;
    summary.coverage = coverage_ratio(overall);
    summary.unique_bugs = bug_count(overall);
    summary.best_fitness = cfg.alpha * summary.coverage
                         + cfg.beta * normalized_bug_score(overall, 10);
    summary.runtime_sec = seconds;
    summary.generations = cfg.generations;
    summary.population_size = cfg.population_size;
    summary.samples_per_individual = cfg.samples_per_individual;
    summary.mutation_rate = cfg.mutation_rate;
    summary.crossover_rate = cfg.crossover_rate;
    summary.novelty_score = 0.0;
    summary.diversity_score = 0.0;
    summary.avg_input_length = avg_input_length(overall);
    return summary;
}

AdaptiveResult run_adaptive_experiment(const Grammar& grammar, const GAConfig& cfg, int run_id) {
    auto start = std::chrono::high_resolution_clock::now();
    std::mt19937 rng(cfg.seed + static_cast<std::uint32_t>(run_id));

    std::vector<Candidate> population;
    population.reserve(cfg.population_size);
    for (int i = 0; i < cfg.population_size; ++i) {
        population.push_back(Candidate{random_individual(grammar, rng), 0.0, {}, {}});
    }

    CorpusStats overall;
    std::array<bool, kCoverageBranches> globally_covered{};
    std::array<std::size_t, kCoverageBranches> global_hit_counts{};
    std::vector<GenerationRecord> log;
    double best_seen = -std::numeric_limits<double>::infinity();
    Individual best_individual = population.front().ind;

    for (int gen = 0; gen < cfg.generations; ++gen) {
        // Evaluate entire population
        for (auto& cand : population) {
            cand = evaluate_candidate(grammar, cfg, cand.ind, rng, overall,
                                     globally_covered, global_hit_counts);
            if (cand.fitness > best_seen) {
                best_seen = cand.fitness;
                best_individual = cand.ind;
            }
        }

        std::sort(population.begin(), population.end(),
                  [](const Candidate& a, const Candidate& b) {
                      return a.fitness > b.fitness;
                  });

        double gen_novelty = 0.0;
        std::size_t prev_covered = 0;
        for (bool b : globally_covered) if (b) prev_covered++;

        GenerationRecord rec;
        rec.run_id = run_id;
        rec.generation = gen;
        rec.best_fitness = population.front().fitness;
        rec.avg_fitness = average_fitness(population);
        rec.best_coverage = coverage_ratio(population.front().stats);
        rec.best_bugs = bug_count(population.front().stats);
        rec.novelty = static_cast<double>(prev_covered) / kCoverageBranches;
        rec.diversity = population_diversity(population);
        rec.avg_input_length = avg_input_length(population.front().stats);
        log.push_back(rec);

        // Build next generation
        std::vector<Candidate> next_gen;
        next_gen.reserve(cfg.population_size);

        // Elitism
        for (int i = 0; i < cfg.elite_count && i < static_cast<int>(population.size()); ++i) {
            next_gen.push_back(population[i]);
        }

        // Adaptive mutation rate: decrease as we converge
        double adaptive_mut = cfg.mutation_rate;
        if (gen > cfg.generations / 2) {
            adaptive_mut = cfg.mutation_rate * 0.7; // fine-tuning phase
        }
        if (gen > 3 * cfg.generations / 4) {
            adaptive_mut = cfg.mutation_rate * 0.4; // exploitation phase
        }

        while (static_cast<int>(next_gen.size()) < cfg.population_size) {
            Individual p1 = tournament_select(population, rng, cfg.tournament_size);
            Individual p2 = tournament_select(population, rng, cfg.tournament_size);
            Individual child = crossover(p1, p2, rng, cfg.crossover_rate);
            mutate(child, rng, adaptive_mut);
            next_gen.push_back(Candidate{child, 0.0, {}, {}});
        }

        population = std::move(next_gen);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    RunSummary summary;
    summary.mode = "adaptive";
    summary.seed = cfg.seed + static_cast<std::uint32_t>(run_id);
    summary.total_inputs = overall.total_inputs;
    summary.coverage = coverage_ratio(overall);
    summary.unique_bugs = bug_count(overall);
    summary.best_fitness = best_seen;
    summary.runtime_sec = seconds;
    summary.generations = cfg.generations;
    summary.population_size = cfg.population_size;
    summary.samples_per_individual = cfg.samples_per_individual;
    summary.mutation_rate = cfg.mutation_rate;
    summary.crossover_rate = cfg.crossover_rate;
    summary.novelty_score = static_cast<double>(std::count(globally_covered.begin(), globally_covered.end(), true)) / kCoverageBranches;
    summary.diversity_score = population_diversity(population);
    summary.avg_input_length = avg_input_length(overall);

    return AdaptiveResult{summary, log, best_individual};
}
