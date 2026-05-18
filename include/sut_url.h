#pragma once

#include "isut.h"

class UrlSUT : public ISut {
public:
    [[nodiscard]] ExecutionResult run(const std::string& input) const override;
    [[nodiscard]] std::size_t branch_count() const noexcept override;
    [[nodiscard]] std::string name() const noexcept override;
};
