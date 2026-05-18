// sut_csv.cpp — embedded CSV parser/validator (40 branches)
#include "sut_csv.h"
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

static constexpr std::size_t kCsvBranches = 40;

enum CsvBranch : std::size_t {
    B_CSV_EMPTY_INPUT = 0, B_CSV_WHITESPACE_ONLY, B_CSV_SINGLE_FIELD,
    B_CSV_MULTI_FIELD, B_CSV_SINGLE_ROW, B_CSV_MULTI_ROW,
    B_CSV_QUOTED_FIELD, B_CSV_UNQUOTED_FIELD, B_CSV_EMPTY_FIELD,
    B_CSV_ESCAPED_QUOTE, B_CSV_FIELD_WITH_COMMA, B_CSV_FIELD_WITH_NEWLINE,
    B_CSV_FIELD_WITH_SPACES,
    B_CSV_NUMERIC_FIELD, B_CSV_FLOAT_FIELD, B_CSV_NEGATIVE_FIELD,
    B_CSV_LONG_FIELD,
    B_CSV_ROW_WIDTH_1, B_CSV_ROW_WIDTH_3, B_CSV_ROW_WIDTH_5, B_CSV_ROW_WIDTH_GT5,
    B_CSV_INCONSISTENT_COLS, B_CSV_HEADER_ROW, B_CSV_NUMERIC_HEADER,
    B_CSV_10_ROWS,
    B_CSV_UNCLOSED_QUOTE, B_CSV_CR_LF, B_CSV_TAB_DELIMITER,
    B_CSV_NULL_BYTE, B_CSV_ONLY_COMMAS, B_CSV_ONLY_QUOTES,
    B_CSV_EMPTY_LAST_FIELD, B_CSV_FIELD_NUM_LARGE,
    B_CSV_NEGATIVE_ZERO, B_CSV_ALL_EMPTY_FIELDS,
    B_CSV_MIXED_TYPES_COL, B_CSV_BOOL_FIELD, B_CSV_DATE_FIELD
};

static bool is_header_token(const std::string& f) {
    static const char* keys[] = {"id", "name", "date", "value", "type"};
    for (const char* k : keys) {
        if (f == k) return true;
    }
    return false;
}

static bool is_bool_field(const std::string& f) {
    return f == "true" || f == "false" || f == "yes" || f == "no";
}

static bool is_date_field(const std::string& f) {
    if (f.size() != 10 || f[4] != '-' || f[7] != '-') return false;
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (!std::isdigit(static_cast<unsigned char>(f[i]))) return false;
    }
    return true;
}

class CsvParser {
public:
    CsvParser(const std::string& s, ExecutionResult& res) : s_(s), res_(res) {
        res_.input_length = s.size();
    }

    void parse() {
        if (s_.empty()) {
            hit(B_CSV_EMPTY_INPUT);
            throw std::runtime_error("EMPTY_INPUT");
        }
        bool all_ws = true;
        for (char c : s_) {
            if (!std::isspace(static_cast<unsigned char>(c))) { all_ws = false; break; }
        }
        if (all_ws) {
            hit(B_CSV_WHITESPACE_ONLY);
            return;
        }
        if (s_.find('\0') != std::string::npos) {
            hit(B_CSV_NULL_BYTE);
            throw std::runtime_error("NULL_BYTE");
        }
        bool only_commas = true;
        for (char c : s_) {
            if (c != ',') only_commas = false;
        }
        if (only_commas) hit(B_CSV_ONLY_COMMAS);
        bool only_quotes = true;
        for (char c : s_) {
            if (c != '"') only_quotes = false;
        }
        if (only_quotes) hit(B_CSV_ONLY_QUOTES);

        std::size_t row_start = 0;
        int row_count = 0;
        int prev_width = -1;
        bool first_row = true;

        while (row_start <= s_.size()) {
            std::size_t row_end = s_.find('\n', row_start);
            if (row_end == std::string::npos) row_end = s_.size();
            std::string row = s_.substr(row_start, row_end - row_start);
            if (row.size() >= 2 && row.back() == '\n') row.pop_back();
            if (row.size() >= 1 && row.back() == '\r') {
                hit(B_CSV_CR_LF);
                row.pop_back();
            }
            if (row.find('\t') != std::string::npos) hit(B_CSV_TAB_DELIMITER);

            if (!row.empty() || row_start < s_.size()) {
                auto fields = parse_row(row);
                int w = static_cast<int>(fields.size());
                if (w == 1) hit(B_CSV_ROW_WIDTH_1);
                if (w == 3) hit(B_CSV_ROW_WIDTH_3);
                if (w == 5) hit(B_CSV_ROW_WIDTH_5);
                if (w > 5) hit(B_CSV_ROW_WIDTH_GT5);
                if (prev_width >= 0 && prev_width != w) hit(B_CSV_INCONSISTENT_COLS);
                prev_width = w;
                row_count++;
                if (row_count == 1 && !fields.empty() && is_header_token(fields[0])) {
                    hit(B_CSV_HEADER_ROW);
                }
                if (row_count == 1 && !fields.empty()) {
                    bool all_digit = !fields[0].empty();
                    for (char c : fields[0]) {
                        if (!std::isdigit(static_cast<unsigned char>(c))) all_digit = false;
                    }
                    if (all_digit) hit(B_CSV_NUMERIC_HEADER);
                }
                if (row_count > 10) hit(B_CSV_10_ROWS);
                (void)first_row;
                first_row = false;
            }

            if (row_end >= s_.size()) break;
            row_start = row_end + 1;
        }

        if (row_count == 1) hit(B_CSV_SINGLE_ROW);
        if (row_count > 1) hit(B_CSV_MULTI_ROW);
    }

private:
    const std::string& s_;
    ExecutionResult& res_;

    void hit(std::size_t id) {
        if (id < kCsvBranches) res_.branch_hits[id] = true;
    }

    std::vector<std::string> parse_row(const std::string& row) {
        std::vector<std::string> fields;
        std::size_t i = 0;
        bool all_empty = true;
        while (i <= row.size()) {
            std::string field;
            if (i < row.size() && row[i] == '"') {
                hit(B_CSV_QUOTED_FIELD);
                ++i;
                while (i < row.size()) {
                    if (row[i] == '"') {
                        if (i + 1 < row.size() && row[i + 1] == '"') {
                            hit(B_CSV_ESCAPED_QUOTE);
                            field += '"';
                            i += 2;
                        } else {
                            ++i;
                            break;
                        }
                    } else {
                        if (row[i] == ',') hit(B_CSV_FIELD_WITH_COMMA);
                        if (row[i] == '\n') hit(B_CSV_FIELD_WITH_NEWLINE);
                        if (row[i] == ' ') hit(B_CSV_FIELD_WITH_SPACES);
                        field += row[i++];
                    }
                }
                if (i > row.size() || (i <= row.size() && i > 0 && row[i - 1] != '"' && row.find('"') != std::string::npos)) {
                    if (row.back() != '"' && row.find('"') == 0) {
                        hit(B_CSV_UNCLOSED_QUOTE);
                        throw std::runtime_error("UNCLOSED_QUOTE");
                    }
                }
            } else {
                std::size_t start = i;
                while (i < row.size() && row[i] != ',') ++i;
                field = row.substr(start, i - start);
                if (field.empty()) {
                    hit(B_CSV_EMPTY_FIELD);
                } else {
                    hit(B_CSV_UNQUOTED_FIELD);
                    all_empty = false;
                }
            }
            if (!field.empty()) all_empty = false;
            if (field.size() > 20) hit(B_CSV_LONG_FIELD);
            classify_field(field);
            fields.push_back(field);
            if (i < row.size() && row[i] == ',') {
                ++i;
                if (i >= row.size()) hit(B_CSV_EMPTY_LAST_FIELD);
            } else {
                break;
            }
        }
        if (fields.size() == 1) hit(B_CSV_SINGLE_FIELD);
        if (fields.size() > 1) hit(B_CSV_MULTI_FIELD);
        if (all_empty && !fields.empty()) hit(B_CSV_ALL_EMPTY_FIELDS);
        return fields;
    }

    void classify_field(const std::string& field) {
        if (field.empty()) return;
        if (field == "-0") hit(B_CSV_NEGATIVE_ZERO);
        if (is_bool_field(field)) hit(B_CSV_BOOL_FIELD);
        if (is_date_field(field)) hit(B_CSV_DATE_FIELD);
        bool numeric = true;
        bool has_dot = false;
        for (char c : field) {
            if (c == '.') has_dot = true;
            else if (c == '-') continue;
            else if (!std::isdigit(static_cast<unsigned char>(c))) numeric = false;
        }
        if (numeric && !field.empty()) {
            if (field[0] == '-') hit(B_CSV_NEGATIVE_FIELD);
            if (has_dot) hit(B_CSV_FLOAT_FIELD);
            else hit(B_CSV_NUMERIC_FIELD);
            try {
                long long v = std::stoll(field);
                if (v > 999999) hit(B_CSV_FIELD_NUM_LARGE);
            } catch (...) {}
        }
    }
};

} // namespace

std::size_t CsvSUT::branch_count() const noexcept { return kCsvBranches; }
std::string CsvSUT::name() const noexcept { return "csv"; }

ExecutionResult CsvSUT::run(const std::string& input) const {
    ExecutionResult result;
    result.branch_count = branch_count();
    try {
        CsvParser p(input, result);
        p.parse();
    } catch (const std::exception& e) {
        result.crashed = true;
        result.crash_signature = e.what();
    }
    return result;
}
