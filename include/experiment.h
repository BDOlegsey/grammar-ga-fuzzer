#pragma once

#include "ga.h"
#include "isut.h"
#include <memory>
#include <string>
#include <vector>

Grammar build_rich_grammar();
Grammar build_json_grammar();
Grammar build_csv_grammar();
Grammar build_url_grammar();

[[nodiscard]] Grammar grammar_for_sut(const std::string& sut_name);

[[nodiscard]] std::unique_ptr<ISut> make_sut(const std::string& name);

void run_experiments(
    const Grammar& grammar,
    const GAConfig& cfg,
    const ISut& sut,
    int runs,
    std::vector<RunSummary>& baseline_runs,
    std::vector<RunSummary>& adaptive_runs,
    std::vector<GenerationRecord>& generation_logs
);
