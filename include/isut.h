// isut.h — абстрактный интерфейс для систем под тестированием
#pragma once

#include <array>
#include <cstddef>
#include <string>

static constexpr std::size_t kMaxBranches = 128;

struct ExecutionResult {
    std::array<bool, kMaxBranches> branch_hits{};
    bool crashed = false;
    std::string crash_signature;
    std::size_t input_length = 0;
    std::size_t branch_count = 0;
};

class ISut {
public:
    virtual ~ISut() = default;
    [[nodiscard]] virtual ExecutionResult run(const std::string& input) const = 0;
    [[nodiscard]] virtual std::size_t branch_count() const noexcept = 0;
    [[nodiscard]] virtual std::string name() const noexcept = 0;
};
