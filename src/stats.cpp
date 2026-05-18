// stats.cpp — statistical utilities for experiment reporting
#include "stats.h"
#include <algorithm>
#include <cmath>
#include <numeric>

std::pair<double, double> mean_and_std(const std::vector<double>& v) {
    if (v.empty()) return {0.0, 0.0};
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    double mean = sum / static_cast<double>(v.size());
    double var = 0.0;
    for (double x : v) {
        double d = x - mean;
        var += d * d;
    }
    var /= static_cast<double>(v.size());
    return {mean, std::sqrt(var)};
}

double cohens_d(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.size() < 2 || b.size() < 2) return 0.0;
    auto [ma, sa] = mean_and_std(a);
    auto [mb, sb] = mean_and_std(b);
    double pooled = ((sa * sa) * (a.size() - 1) + (sb * sb) * (b.size() - 1))
                  / static_cast<double>(a.size() + b.size() - 2);
    if (pooled <= 0.0) return 0.0;
    return (ma - mb) / std::sqrt(pooled);
}

double welch_t_stat(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.size() < 2 || b.size() < 2) return 0.0;
    auto [ma, sa] = mean_and_std(a);
    auto [mb, sb] = mean_and_std(b);
    double va = sa * sa / static_cast<double>(a.size());
    double vb = sb * sb / static_cast<double>(b.size());
    double denom = std::sqrt(va + vb);
    if (denom <= 0.0) return 0.0;
    return (ma - mb) / denom;
}

namespace {

double log_gamma(double x) {
    static const double coef[] = {
        76.18009172947146, -86.50532032941677, 24.01409824083091,
        -1.231739572450155, 0.1208650973866179e-2, -0.5395239384953e-5
    };
    double y = x;
    double tmp = x + 5.5;
    tmp -= (x + 0.5) * std::log(tmp);
    double ser = 1.000000000190015;
    for (double c : coef) {
        y += 1.0;
        ser += c / y;
    }
    return -tmp + std::log(2.5066282746310005 * ser / x);
}

double betacf(double a, double b, double x) {
    constexpr int max_iter = 200;
    constexpr double eps = 3.0e-7;
    double am = 1.0;
    double bm = 1.0;
    double az = 1.0;
    double qab = a + b;
    double qap = a + 1.0;
    double qam = a - 1.0;
    double bz = 1.0 - qab * x / qap;
    for (int m = 1; m <= max_iter; ++m) {
        double em = static_cast<double>(m);
        double tem = em + em;
        double d = em * (b - em) * x / ((qam + tem) * (a + tem));
        double ap = az + d * am;
        double bp = bz + d * bm;
        d = -(a + em) * (qab + em) * x / ((a + tem) * (qap + tem));
        double app = ap + d * az;
        double bpp = bp + d * bz;
        am = ap / bpp;
        bm = bp / bpp;
        az = app / bpp;
        bz = 1.0;
        if (std::fabs(app - az) < eps * std::fabs(az)) break;
    }
    return az;
}

double betai(double a, double b, double x) {
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;
    double bt = std::exp(log_gamma(a + b) - log_gamma(a) - log_gamma(b) + a * std::log(x) + b * std::log(1.0 - x));
    return bt * betacf(a, b, x) / a;
}

double welch_df(const std::vector<double>& a, const std::vector<double>& b) {
    auto [ma, sa] = mean_and_std(a);
    auto [mb, sb] = mean_and_std(b);
    double va = sa * sa / static_cast<double>(a.size());
    double vb = sb * sb / static_cast<double>(b.size());
    double num = (va + vb) * (va + vb);
    double den = (va * va) / static_cast<double>(a.size() - 1)
               + (vb * vb) / static_cast<double>(b.size() - 1);
    if (den <= 0.0) return 1.0;
    return num / den;
}

} // namespace

double welch_p_value(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.size() < 2 || b.size() < 2) return 1.0;
    double t = welch_t_stat(a, b);
    double df = welch_df(a, b);
    double x = df / (df + t * t);
    return 2.0 * betai(df / 2.0, 0.5, x);
}

double cliffs_delta(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.empty() || b.empty()) return 0.0;
    double greater = 0.0;
    double less = 0.0;
    for (double x : a) {
        for (double y : b) {
            if (x > y) greater += 1.0;
            else if (x < y) less += 1.0;
        }
    }
    double total = static_cast<double>(a.size() * b.size());
    return (greater - less) / total;
}

MannWhitneyResult mann_whitney(const std::vector<double>& a, const std::vector<double>& b) {
    MannWhitneyResult result;
    if (a.empty() || b.empty()) return result;

    std::vector<std::pair<double, int>> combined;
    combined.reserve(a.size() + b.size());
    for (double x : a) combined.push_back({x, 0});
    for (double x : b) combined.push_back({x, 1});
    std::sort(combined.begin(), combined.end(),
              [](const auto& p1, const auto& p2) { return p1.first < p2.first; });

    double rank_sum_a = 0.0;
    for (std::size_t i = 0; i < combined.size(); ++i) {
        if (combined[i].second == 0) {
            rank_sum_a += static_cast<double>(i + 1);
        }
    }

    double n1 = static_cast<double>(a.size());
    double n2 = static_cast<double>(b.size());
    result.U1 = rank_sum_a - n1 * (n1 + 1.0) / 2.0;
    result.U2 = n1 * n2 - result.U1;
    double u = std::min(result.U1, result.U2);
    double mu = n1 * n2 / 2.0;
    double sigma = std::sqrt(n1 * n2 * (n1 + n2 + 1.0) / 12.0);
    if (sigma > 0.0) {
        double z = (u - mu) / sigma;
        result.p_approx = 2.0 * (1.0 - 0.5 * (1.0 + std::erf(std::fabs(z) / std::sqrt(2.0))));
    }
    return result;
}
