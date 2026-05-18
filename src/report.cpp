// report.cpp — formatted experiment result output
#include "report.h"
#include "stats.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>

namespace {

template <typename T, typename Getter>
std::pair<double, double> mean_std(const std::vector<T>& data, Getter getter) {
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

} // namespace

void print_group_summary(const std::string& title,
                         const std::vector<RunSummary>& runs,
                         std::ostream& out) {
    auto [cov_mean, cov_std] = mean_std(runs, [](const RunSummary& r) { return r.coverage; });
    auto [bug_mean, bug_std] = mean_std(runs, [](const RunSummary& r) { return static_cast<double>(r.unique_bugs); });
    auto [time_mean, time_std] = mean_std(runs, [](const RunSummary& r) { return r.runtime_sec; });
    auto [fit_mean, fit_std] = mean_std(runs, [](const RunSummary& r) { return r.best_fitness; });
    auto [nov_mean, nov_std] = mean_std(runs, [](const RunSummary& r) { return r.novelty_score; });
    auto [motion_mean, div_std] = mean_std(runs, [](const RunSummary& r) { return r.diversity_score; });

    out << "\n[" << title << "]\n";
    out << std::fixed << std::setprecision(4);
    out << "coverage:       " << cov_mean << " +/- " << cov_std << "\n";
    out << "bugs:           " << bug_mean << " +/- " << bug_std << "\n";
    out << "runtime s:      " << time_mean << " +/- " << time_std << "\n";
    out << "fitness:        " << fit_mean << " +/- " << fit_std << "\n";
    out << "novelty:        " << nov_mean << " +/- " << nov_std << "\n";
    out << "diversity:      " << motion_mean << " +/- " << div_std << "\n";
}

void print_comparison(const std::string& title,
                      const std::vector<RunSummary>& baseline,
                      const std::vector<RunSummary>& adaptive,
                      std::ostream& out) {
    out << "\n[COMPARISON: " << title << "]\n";
    out << std::fixed << std::setprecision(4);
    out << "Metric         Baseline       Adaptive       Delta     t      p       d      d\n";

    auto print_row = [&](const char* name,
                         auto getter) {
        std::vector<double> bv, av;
        for (const auto& r : baseline) bv.push_back(getter(r));
        for (const auto& r : adaptive) av.push_back(getter(r));
        auto [bm, bs] = mean_and_std(bv);
        auto [am, as] = mean_and_std(av);
        double delta = (bm != 0.0) ? (am - bm) / bm * 100.0 : 0.0;
        double t = welch_t_stat(bv, av);
        double p = welch_p_value(bv, av);
        double d = cohens_d(bv, av);
        double cd = cliffs_delta(bv, av);
        out << std::left << std::setw(15) << name
            << std::setw(15) << (std::to_string(bm).substr(0, 6) + "+/-" + std::to_string(bs).substr(0, 4))
            << std::setw(15) << (std::to_string(am).substr(0, 6) + "+/-" + std::to_string(as).substr(0, 4))
            << std::setw(10) << (std::to_string(delta).substr(0, 6) + "%")
            << std::setw(7) << t
            << std::setw(8) << p
            << std::setw(7) << d
            << std::setw(7) << cd << "\n";
    };

    print_row("coverage", [](const RunSummary& r) { return r.coverage; });
    print_row("unique_bugs", [](const RunSummary& r) { return static_cast<double>(r.unique_bugs); });
    print_row("best_fitness", [](const RunSummary& r) { return r.best_fitness; });

    std::vector<double> bc, ac;
    for (const auto& r : baseline) bc.push_back(r.coverage);
    for (const auto& r : adaptive) ac.push_back(r.coverage);
    auto mw = mann_whitney(bc, ac);
    out << "Mann-Whitney U (coverage): U=" << mw.U1 << " p_approx=" << mw.p_approx << "\n";
}
