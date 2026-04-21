#include "utils.h"
#include <algorithm>
#include <cmath>
#include <numeric>

double clamp01(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

std::vector<double> normalize(const std::vector<double>& probs) {
    std::vector<double> res;
    if (probs.empty()) return res;

    double sum = std::accumulate(probs.begin(), probs.end(), 0.0);
    if (sum <= 0.0) {
        res.assign(probs.size(), 1.0 / static_cast<double>(probs.size()));
        return res;
    }

    res.reserve(probs.size());
    for (double p : probs) {
        res.push_back(p / sum);
    }
    return res;
}

std::vector<double> softmax(const std::vector<double>& vals, double temp) {
    std::vector<double> res;
    if (vals.empty()) return res;

    double max_val = *std::max_element(vals.begin(), vals.end());
    double sum = 0.0;
    std::vector<double> exps;
    exps.reserve(vals.size());
    for (double v : vals) {
        double e = std::exp((v - max_val) / temp);
        exps.push_back(e);
        sum += e;
    }
    if (sum <= 0.0) {
        res.assign(vals.size(), 1.0 / static_cast<double>(vals.size()));
        return res;
    }
    for (double e : exps) {
        res.push_back(e / sum);
    }
    return res;
}
