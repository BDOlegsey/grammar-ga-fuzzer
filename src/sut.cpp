// sut.cpp — MiniSUT arithmetic parser (60 branches)
#include "sut.h"
#include "common.h"
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace {

// 55 coverage points: common, medium, and ultra-rare pattern branches
enum BranchId : std::size_t {
    // === Common branches (0-24) ===
    B_EMPTY_INPUT = 0,
    B_WHITESPACE_ONLY = 1,
    B_EXPR_UNARY_PLUS = 2,
    B_EXPR_UNARY_MINUS = 3,
    B_EXPR_UNARY_NOT = 4,
    B_EXPR_UNARY_BITNOT = 5,
    B_BINARY_ADD = 6,
    B_BINARY_SUB = 7,
    B_BINARY_MUL = 8,
    B_BINARY_DIV = 9,
    B_BINARY_MOD = 10,
    B_BINARY_LT = 11,
    B_BINARY_GT = 12,
    B_BINARY_EQ = 13,
    B_BINARY_NEQ = 14,
    B_BINARY_AND = 15,
    B_BINARY_OR = 16,
    B_TERM_NO_BINOP = 17,
    B_FACTOR_PAREN = 18,
    B_FACTOR_NUMBER_INT = 19,
    B_FACTOR_NUMBER_FLOAT = 20,
    B_FACTOR_NUMBER_HEX = 21,
    B_FACTOR_NUMBER_SCI = 22,
    B_FACTOR_FUNC_SIN = 23,
    B_FACTOR_FUNC_COS = 24,

    // === Medium rarity (25-39) ===
    B_FACTOR_FUNC_ABS = 25,
    B_FACTOR_FUNC_SQRT = 26,
    B_FACTOR_VAR_X = 27,
    B_FACTOR_VAR_Y = 28,
    B_FACTOR_VAR_Z = 29,
    B_TERNARY_COND = 30,
    B_TERNARY_TRUE_BRANCH = 31,
    B_TERNARY_FALSE_BRANCH = 32,
    B_NUMBER_LEADING_ZERO = 33,
    B_NUMBER_TOO_LARGE = 34,
    B_NUMBER_SPECIAL_13 = 35,
    B_NUMBER_SPECIAL_42 = 36,
    B_NUMBER_SPECIAL_666 = 37,
    B_DEEP_NESTING_5 = 38,
    B_FUNC_DOMAIN_ERROR = 39,

    // === Error/edge branches (40-43) ===
    B_TRAILING_GARBAGE = 40,
    B_DIV_BY_ZERO = 41,
    B_MOD_BY_ZERO = 42,
    B_EMPTY_PARENS = 43,

    // === Rare pattern branches (44-54) — require specific combinations ===
    B_PATTERN_UNARY_CHAIN = 44,     // 3+ unary ops: !!!x, ~~-5
    B_PATTERN_FUNC_OF_ZERO = 45,    // func(0) — arg evals to 0
    B_PATTERN_SELF_SUB_ZERO = 46,   // (x-x) or (5-5) = 0
    B_PATTERN_TERNARY_ARITH = 47,   // ternary inside arithmetic: 5+(1?2:3)
    B_PATTERN_NESTED_PAREN_4 = 48,  // ((((expr))))
    B_PATTERN_MULTI_FUNC = 49,      // sin(...)+cos(...)
    B_PATTERN_CMP_ARITH = 50,       // (1<2)+5 — comparison in arithmetic
    B_PATTERN_HEX_ARITH = 51,       // 0x1a+5 — hex in arithmetic
    B_PATTERN_SCI_CMP = 52,         // 1e5<2 — scientific in comparison
    B_PATTERN_ALL_VARS = 53,        // expression uses x, y, and z
    B_PATTERN_LONG_EVAL = 54,       // >40 chars, evaluates without error

    // === Ultra-rare branches (55-59) — very specific structure ===
    B_PATTERN_FUNC_CHAIN_3 = 55,    // sin(cos(sin(x))) — 3+ funcs deep
    B_PATTERN_MULTI_TERNARY = 56,   // a?b:c?d:e — two ternaries
    B_PATTERN_OP_PRECEDENCE = 57,   // 1+2*3+4 — mixed precedence eval
    B_PATTERN_DIV_TO_ONE = 58,      // x/x or y/y
    B_PATTERN_NEG_OF_NEG = 59       // -(-x) or -(-(-5))
};

class Parser {
public:
    Parser(const std::string& input, ExecutionResult& result)
        : s_(input), res_(result) {
        res_.input_length = input.size();
    }

    double parse() {
        skip_ws();
        if (eof()) {
            hit(B_EMPTY_INPUT);
            throw std::runtime_error("EMPTY_INPUT");
        }

        double v = parse_ternary();
        skip_ws();

        if (!eof()) {
            hit(B_TRAILING_GARBAGE);
            throw std::runtime_error("TRAILING_GARBAGE");
        }

        // Pattern: long successful evaluation
        if (s_.length() > 40) {
            hit(B_PATTERN_LONG_EVAL);
        }

        // Pattern: all three vars used
        if (vars_used_.count('x') && vars_used_.count('y') && vars_used_.count('z')) {
            hit(B_PATTERN_ALL_VARS);
        }

        // Pattern: multiple different functions
        if (funcs_used_.size() >= 2) {
            hit(B_PATTERN_MULTI_FUNC);
        }

        // Pattern: multi-ternary
        if (ternary_count_ > 1) {
            hit(B_PATTERN_MULTI_TERNARY);
        }

        // Pattern: func chain 3+ deep
        if (func_nesting_max_ >= 3) {
            hit(B_PATTERN_FUNC_CHAIN_3);
        }

        // Pattern: operator precedence (mixed + and *)
        if (saw_add_sub_ && saw_mul_div_) {
            hit(B_PATTERN_OP_PRECEDENCE);
        }

        return v;
    }

private:
    const std::string& s_;
    std::size_t pos_{0};
    int nesting_{0};
    int func_nesting_{0};
    int ternary_nesting_{0};
    int paren_nesting_{0};
    int unary_chain_{0};
    int ternary_count_{0};
    int func_nesting_max_{0};
    int neg_count_{0};
    bool saw_add_sub_{false};
    bool saw_mul_div_{false};
    bool in_arithmetic_{false};
    bool saw_cmp_in_arith_{false};
    bool saw_hex_in_arith_{false};
    bool saw_sci_in_cmp_{false};
    bool saw_ternary_in_arith_{false};
    std::unordered_set<char> vars_used_;
    std::unordered_set<std::string> funcs_used_;
    ExecutionResult& res_;

    void hit(std::size_t id) {
        if (id < res_.branch_hits.size()) {
            res_.branch_hits[id] = true;
        }
    }

    bool eof() const { return pos_ >= s_.size(); }
    char peek() const { return eof() ? '\0' : s_[pos_]; }
    char get() { return eof() ? '\0' : s_[pos_++]; }

    void skip_ws() {
        bool moved = false;
        while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
            get();
            moved = true;
        }
        if (moved && eof()) {
            hit(B_WHITESPACE_ONLY);
        }
    }

    bool match(char c) {
        if (!eof() && peek() == c) {
            get();
            return true;
        }
        return false;
    }

    bool match_str(const std::string& str) {
        if (pos_ + str.size() > s_.size()) return false;
        if (s_.compare(pos_, str.size(), str) == 0) {
            pos_ += str.size();
            return true;
        }
        return false;
    }

    void check_nesting() {
        if (nesting_ > 5) {
            hit(B_DEEP_NESTING_5);
        }
        if (nesting_ > 12) {
            throw std::runtime_error("STACK_DEPTH_EXCEEDED");
        }
    }

    void check_special_value(double v) {
        long long iv = static_cast<long long>(std::llround(v));
        if (iv == 13) hit(B_NUMBER_SPECIAL_13);
        if (iv == 42) hit(B_NUMBER_SPECIAL_42);
        if (iv == 666) hit(B_NUMBER_SPECIAL_666);
    }

    void check_zero_patterns(double arg_val) {
        if (arg_val == 0.0) {
            hit(B_PATTERN_FUNC_OF_ZERO);
        }
    }

    void check_self_sub_pattern(double lhs, double rhs) {
        if (lhs == rhs && in_arithmetic_) {
            hit(B_PATTERN_SELF_SUB_ZERO);
        }
    }

    // === Ternary ===
    double parse_ternary() {
        double cond = parse_or();
        skip_ws();
        if (match('?')) {
            hit(B_TERNARY_COND);
            ternary_nesting_++;
            ternary_count_++;

            if (in_arithmetic_) {
                saw_ternary_in_arith_ = true;
            }

            double true_val = parse_ternary();
            skip_ws();
            if (!match(':')) {
                throw std::runtime_error("MISSING_COLON_TERNARY");
            }
            if (cond != 0.0) {
                hit(B_TERNARY_TRUE_BRANCH);
                (void)parse_ternary();
                ternary_nesting_--;
                return true_val;
            } else {
                hit(B_TERNARY_FALSE_BRANCH);
                double v = parse_ternary();
                ternary_nesting_--;
                return v;
            }
        }
        return cond;
    }

    // === OR ===
    double parse_or() {
        double v = parse_and();
        while (true) {
            skip_ws();
            if (match_str("||")) {
                hit(B_BINARY_OR);
                double rhs = parse_and();
                v = (v != 0.0 || rhs != 0.0) ? 1.0 : 0.0;
            } else {
                break;
            }
        }
        return v;
    }

    // === AND ===
    double parse_and() {
        double v = parse_equality();
        while (true) {
            skip_ws();
            if (match_str("&&")) {
                hit(B_BINARY_AND);
                double rhs = parse_equality();
                v = (v != 0.0 && rhs != 0.0) ? 1.0 : 0.0;
            } else {
                break;
            }
        }
        return v;
    }

    // === Equality ===
    double parse_equality() {
        double v = parse_relational();
        while (true) {
            skip_ws();
            if (match_str("==")) {
                hit(B_BINARY_EQ);
                v = (v == parse_relational()) ? 1.0 : 0.0;
            } else if (match_str("!=")) {
                hit(B_BINARY_NEQ);
                v = (v != parse_relational()) ? 1.0 : 0.0;
            } else {
                break;
            }
        }
        return v;
    }

    // === Relational (with SCI and CMP-IN-ARITH pattern tracking) ===
    double parse_relational() {
        double v = parse_additive();
        while (true) {
            skip_ws();
            if (match('<')) {
                hit(B_BINARY_LT);
                if (in_arithmetic_) saw_cmp_in_arith_ = true;
                bool was_sci = last_was_sci_;
                double rhs = parse_additive();
                if (was_sci || last_was_sci_) saw_sci_in_cmp_ = true;
                v = (v < rhs) ? 1.0 : 0.0;
            } else if (match('>')) {
                hit(B_BINARY_GT);
                if (in_arithmetic_) saw_cmp_in_arith_ = true;
                bool was_sci = last_was_sci_;
                double rhs = parse_additive();
                if (was_sci || last_was_sci_) saw_sci_in_cmp_ = true;
                v = (v > rhs) ? 1.0 : 0.0;
            } else {
                break;
            }
        }
        return v;
    }

    // === Additive ===
    double parse_additive() {
        double v = parse_multiplicative();
        bool saw_op = false;
        while (true) {
            skip_ws();
            if (match('+')) {
                hit(B_BINARY_ADD);
                saw_add_sub_ = true;
                saw_op = true;
                v += parse_multiplicative();
            } else if (match('-')) {
                hit(B_BINARY_SUB);
                saw_add_sub_ = true;
                saw_op = true;
                double rhs = parse_multiplicative();
                check_self_sub_pattern(v, rhs);
                v -= rhs;
            } else {
                break;
            }
        }
        if (!saw_op) hit(B_TERM_NO_BINOP);

        // Check pattern: comparison in arithmetic context
        if (saw_cmp_in_arith_) {
            hit(B_PATTERN_CMP_ARITH);
            saw_cmp_in_arith_ = false;
        }
        // Check pattern: ternary in arithmetic
        if (saw_ternary_in_arith_) {
            hit(B_PATTERN_TERNARY_ARITH);
            saw_ternary_in_arith_ = false;
        }
        // Check pattern: hex in arithmetic
        if (saw_hex_in_arith_) {
            hit(B_PATTERN_HEX_ARITH);
            saw_hex_in_arith_ = false;
        }
        // Check pattern: scientific in comparison
        if (saw_sci_in_cmp_) {
            hit(B_PATTERN_SCI_CMP);
            saw_sci_in_cmp_ = false;
        }

        return v;
    }

    // === Multiplicative ===
    double parse_multiplicative() {
        double v = parse_unary();
        while (true) {
            skip_ws();
            if (match('*')) {
                hit(B_BINARY_MUL);
                saw_mul_div_ = true;
                v *= parse_unary();
            } else if (match('/')) {
                hit(B_BINARY_DIV);
                saw_mul_div_ = true;
                double rhs = parse_unary();
                if (rhs == 0.0) {
                    hit(B_DIV_BY_ZERO);
                    throw std::runtime_error("DIV_BY_ZERO");
                }
                v /= rhs;
            } else if (match('%')) {
                hit(B_BINARY_MOD);
                saw_mul_div_ = true;
                double rhs = parse_unary();
                if (rhs == 0.0) {
                    hit(B_MOD_BY_ZERO);
                    throw std::runtime_error("MOD_BY_ZERO");
                }
                v = std::fmod(v, rhs);
            } else {
                break;
            }
        }
        return v;
    }

    // === Unary (with chain tracking) ===
    double parse_unary() {
        skip_ws();
        if (match('+')) {
            hit(B_EXPR_UNARY_PLUS);
            unary_chain_++;
            double v = parse_unary();
            check_unary_chain();
            return +v;
        }
        if (match('-')) {
            hit(B_EXPR_UNARY_MINUS);
            unary_chain_++;
            neg_count_++;
            double v = parse_unary();
            check_unary_chain();
            return -v;
        }
        if (match('!')) {
            hit(B_EXPR_UNARY_NOT);
            unary_chain_++;
            double v = parse_unary();
            check_unary_chain();
            return v == 0.0 ? 1.0 : 0.0;
        }
        if (match('~')) {
            hit(B_EXPR_UNARY_BITNOT);
            unary_chain_++;
            double v = parse_unary();
            check_unary_chain();
            return static_cast<double>(~static_cast<long long>(v));
        }
        unary_chain_ = 0;
        return parse_postfix();
    }

    void check_unary_chain() {
        if (unary_chain_ >= 3) {
            hit(B_PATTERN_UNARY_CHAIN);
            unary_chain_ = 0; // reset to avoid re-hitting
        }
    }

    // === Postfix: functions and variables ===
    double parse_postfix() {
        skip_ws();

        if (match_str("sin")) {
            hit(B_FACTOR_FUNC_SIN);
            funcs_used_.insert("sin");
            return parse_func_arg();
        }
        if (match_str("cos")) {
            hit(B_FACTOR_FUNC_COS);
            funcs_used_.insert("cos");
            return parse_func_arg();
        }
        if (match_str("abs")) {
            hit(B_FACTOR_FUNC_ABS);
            funcs_used_.insert("abs");
            skip_ws();
            if (!match('(')) throw std::runtime_error("EXPECTED_PAREN");
            double v = parse_ternary();
            skip_ws();
            if (!match(')')) throw std::runtime_error("MISSING_CLOSE_PAREN");
            return std::abs(v);
        }
        if (match_str("sqrt")) {
            hit(B_FACTOR_FUNC_SQRT);
            funcs_used_.insert("sqrt");
            skip_ws();
            if (!match('(')) throw std::runtime_error("EXPECTED_PAREN");
            double v = parse_ternary();
            skip_ws();
            if (!match(')')) throw std::runtime_error("MISSING_CLOSE_PAREN");
            if (v < 0.0) {
                hit(B_FUNC_DOMAIN_ERROR);
                throw std::runtime_error("SQRT_DOMAIN_ERROR");
            }
            return std::sqrt(v);
        }

        if (match('x')) {
            vars_used_.insert('x');
            hit(B_FACTOR_VAR_X);
            if (neg_count_ >= 2) hit(B_PATTERN_NEG_OF_NEG);
            neg_count_ = 0;
            return 5.0;
        }
        if (match('y')) {
            vars_used_.insert('y');
            hit(B_FACTOR_VAR_Y);
            if (neg_count_ >= 2) hit(B_PATTERN_NEG_OF_NEG);
            neg_count_ = 0;
            return 7.0;
        }
        if (match('z')) {
            vars_used_.insert('z');
            hit(B_FACTOR_VAR_Z);
            if (neg_count_ >= 2) hit(B_PATTERN_NEG_OF_NEG);
            neg_count_ = 0;
            return 13.0;
        }

        return parse_primary();
    }

    double parse_func_arg() {
        skip_ws();
        if (!match('(')) throw std::runtime_error("EXPECTED_PAREN");
        func_nesting_++;
        func_nesting_max_ = std::max(func_nesting_max_, func_nesting_);
        double v = parse_ternary();
        skip_ws();
        if (!match(')')) throw std::runtime_error("MISSING_CLOSE_PAREN");
        func_nesting_--;
        check_zero_patterns(v);
        return v;
    }

    // === Primary: parens and numbers ===
    double parse_primary() {
        skip_ws();

        if (match('(')) {
            hit(B_FACTOR_PAREN);
            nesting_++;
            paren_nesting_++;
            check_nesting();

            if (paren_nesting_ >= 4) {
                hit(B_PATTERN_NESTED_PAREN_4);
            }

            skip_ws();
            if (match(')')) {
                nesting_--;
                paren_nesting_--;
                hit(B_EMPTY_PARENS);
                throw std::runtime_error("EMPTY_PARENS");
            }

            bool old_arith = in_arithmetic_;
            in_arithmetic_ = true;
            double v = parse_ternary();
            in_arithmetic_ = old_arith;
            skip_ws();
            if (!match(')')) throw std::runtime_error("MISSING_CLOSE_PAREN");
            nesting_--;
            paren_nesting_--;
            check_special_value(v);
            return v;
        }

        return parse_number();
    }

    // === Number parsing ===
    bool last_was_sci_ = false;
    bool last_was_hex_ = false;

    double parse_number() {
        skip_ws();
        last_was_sci_ = false;
        last_was_hex_ = false;

        if (peek() == '0' && pos_ + 1 < s_.size() && s_[pos_ + 1] == 'x') {
            get(); get();
            double v = parse_hex();
            if (in_arithmetic_) saw_hex_in_arith_ = true;
            return v;
        }

        std::size_t start = pos_;
        bool has_digits = false;

        while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
            get();
            has_digits = true;
        }

        bool is_float = false;
        if (!eof() && peek() == '.') {
            get();
            is_float = true;
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                get();
                has_digits = true;
            }
        }

        bool is_sci = false;
        if (has_digits && !eof() && (peek() == 'e' || peek() == 'E')) {
            std::size_t save = pos_;
            get();
            if (!eof() && (peek() == '+' || peek() == '-')) get();
            bool has_exp_digits = false;
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                get();
                has_exp_digits = true;
            }
            if (has_exp_digits) {
                is_sci = true;
            } else {
                pos_ = save;
            }
        }

        std::size_t len = pos_ - start;
        if (len == 0 || !has_digits) {
            throw std::runtime_error("UNEXPECTED_TOKEN");
        }

        if (len > 12) {
            hit(B_NUMBER_TOO_LARGE);
            throw std::runtime_error("NUMBER_TOO_LARGE");
        }

        std::string token = s_.substr(start, len);

        if (!is_float && !is_sci && s_[start] == '0' && len > 1) {
            hit(B_NUMBER_LEADING_ZERO);
        }

        double value = std::stod(token);

        if (is_sci) {
            hit(B_FACTOR_NUMBER_SCI);
            last_was_sci_ = true;
        } else if (is_float) {
            hit(B_FACTOR_NUMBER_FLOAT);
        } else {
            hit(B_FACTOR_NUMBER_INT);
        }

        check_special_value(value);
        return value;
    }

    double parse_hex() {
        std::size_t start = pos_;
        while (!eof() && std::isxdigit(static_cast<unsigned char>(peek()))) get();
        std::size_t len = pos_ - start;
        if (len == 0) throw std::runtime_error("INVALID_HEX");
        if (len > 8) {
            hit(B_NUMBER_TOO_LARGE);
            throw std::runtime_error("NUMBER_TOO_LARGE");
        }
        hit(B_FACTOR_NUMBER_HEX);
        last_was_hex_ = true;
        std::string token = s_.substr(start, len);
        long long val = std::stoll(token, nullptr, 16);
        double d = static_cast<double>(val);
        check_special_value(d);
        return d;
    }
};

} // namespace

std::size_t MiniSUT::branch_count() const noexcept {
    return kCoverageBranches;
}

std::string MiniSUT::name() const noexcept {
    return "arithmetic";
}

ExecutionResult MiniSUT::run(const std::string& input) const {
    ExecutionResult result;
    result.branch_count = branch_count();
    try {
        Parser parser(input, result);
        (void)parser.parse();
    } catch (const std::exception& e) {
        result.crashed = true;
        result.crash_signature = e.what();
    } catch (...) {
        result.crashed = true;
        result.crash_signature = "UNKNOWN_EXCEPTION";
    }
    return result;
}
