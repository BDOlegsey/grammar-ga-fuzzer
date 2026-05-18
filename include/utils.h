#pragma once

#include "grammar.h"
#include <vector>

double clamp01(double x);
std::vector<double> normalize(const std::vector<double>& probs);
std::vector<double> softmax(const std::vector<double>& vals, double temp = 1.0);
