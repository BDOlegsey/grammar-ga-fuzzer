// stats.h — статистические утилиты для отчётности эксперимента
#pragma once

#include <utility>
#include <vector>

[[nodiscard]] std::pair<double, double> mean_and_std(const std::vector<double>& v);
[[nodiscard]] double cohens_d(const std::vector<double>& a, const std::vector<double>& b);
[[nodiscard]] double welch_t_stat(const std::vector<double>& a, const std::vector<double>& b);
[[nodiscard]] double welch_p_value(const std::vector<double>& a, const std::vector<double>& b);
[[nodiscard]] double cliffs_delta(const std::vector<double>& a, const std::vector<double>& b);

struct MannWhitneyResult {
    double U1{0.0};
    double U2{0.0};
    double p_approx{0.0};
};

[[nodiscard]] MannWhitneyResult mann_whitney(const std::vector<double>& a,
                                              const std::vector<double>& b);
