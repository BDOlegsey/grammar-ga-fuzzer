// report.h — форматированный вывод результатов эксперимента
#pragma once

#include "common.h"
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

void print_group_summary(const std::string& title,
                         const std::vector<RunSummary>& runs,
                         std::ostream& out = std::cout);

void print_comparison(const std::string& title,
                      const std::vector<RunSummary>& baseline,
                      const std::vector<RunSummary>& adaptive,
                      std::ostream& out = std::cout);
