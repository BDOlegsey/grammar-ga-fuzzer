// sut_json.cpp — embedded JSON validator/parser (48 branches)
#include "sut_json.h"
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace {

static constexpr std::size_t kJsonBranches = 48;
static constexpr int kJsonMaxDepth = 10;

enum JsonBranch : std::size_t {
    B_JSON_NULL = 0, B_JSON_TRUE, B_JSON_FALSE,
    B_JSON_INT, B_JSON_FLOAT, B_JSON_SCI_NUMBER, B_JSON_NEGATIVE_NUM, B_JSON_ZERO,
    B_JSON_STRING_EMPTY, B_JSON_STRING_NORMAL, B_JSON_ESCAPE_QUOTE,
    B_JSON_ESCAPE_BACKSLASH, B_JSON_ESCAPE_NEWLINE,
    B_JSON_ARRAY_EMPTY, B_JSON_ARRAY_ONE, B_JSON_ARRAY_MULTI,
    B_JSON_OBJECT_EMPTY, B_JSON_OBJECT_ONE_PAIR, B_JSON_OBJECT_MULTI_PAIR,
    B_JSON_NESTED_OBJECT, B_JSON_NESTED_ARRAY, B_JSON_MIXED_NESTED,
    B_JSON_DEPTH_3, B_JSON_DEPTH_5,
    B_JSON_KEY_LONG, B_JSON_KEY_NUMERIC, B_JSON_KEY_COLLISION,
    B_JSON_VALUE_LARGE_INT, B_JSON_VALUE_NEG_INT, B_JSON_VALUE_ZERO_INT,
    B_JSON_ARRAY_NESTED_OBJ, B_JSON_OBJECT_ARRAY_VAL,
    B_JSON_NULL_IN_ARRAY, B_JSON_BOOL_IN_ARRAY, B_JSON_MIXED_TYPES_ARRAY,
    B_JSON_UNICODE_ESCAPE,
    B_JSON_TRAILING_COMMA, B_JSON_MISSING_COLON, B_JSON_UNCLOSED_OBJECT,
    B_JSON_UNCLOSED_ARRAY, B_JSON_UNCLOSED_STRING, B_JSON_EXTRA_COMMA,
    B_JSON_INVALID_ESCAPE, B_JSON_NUMBER_LEADING_ZERO,
    B_JSON_EMPTY_INPUT, B_JSON_WHITESPACE_ONLY
};

class JsonParser {
public:
    JsonParser(const std::string& s, ExecutionResult& res) : s_(s), res_(res) {
        res_.input_length = s.size();
    }

    void parse() {
        skip_ws();
        if (eof()) {
            hit(B_JSON_EMPTY_INPUT);
            throw std::runtime_error("EMPTY_INPUT");
        }
        if (only_ws_seen_ && eof()) {
            hit(B_JSON_WHITESPACE_ONLY);
        }
        parse_value(0);
        skip_ws();
        if (!eof()) throw std::runtime_error("UNEXPECTED_TOKEN");
    }

private:
    const std::string& s_;
    std::size_t pos_{0};
    int depth_{0};
    bool only_ws_seen_{true};
    std::unordered_set<std::string> keys_;
    ExecutionResult& res_;

    void hit(std::size_t id) {
        if (id < kJsonBranches) res_.branch_hits[id] = true;
    }

    bool eof() const { return pos_ >= s_.size(); }
    char peek() const { return eof() ? '\0' : s_[pos_]; }
    char get() { return eof() ? '\0' : s_[pos_++]; }

    void skip_ws() {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
            get();
            only_ws_seen_ = false;
        }
    }

    void check_depth() {
        if (depth_ >= 3) hit(B_JSON_DEPTH_3);
        if (depth_ >= 5) hit(B_JSON_DEPTH_5);
        if (depth_ > kJsonMaxDepth) throw std::runtime_error("DEPTH_EXCEEDED");
    }

    void parse_value(int ctx) {
        skip_ws();
        if (match_str("null")) { hit(B_JSON_NULL); if (ctx == 1) hit(B_JSON_NULL_IN_ARRAY); return; }
        if (match_str("true")) { hit(B_JSON_TRUE); if (ctx == 1) hit(B_JSON_BOOL_IN_ARRAY); return; }
        if (match_str("false")) { hit(B_JSON_FALSE); if (ctx == 1) hit(B_JSON_BOOL_IN_ARRAY); return; }
        if (peek() == '"') { parse_string(); return; }
        if (peek() == '[') { parse_array(); return; }
        if (peek() == '{') { parse_object(); return; }
        if (peek() == '-' || std::isdigit(static_cast<unsigned char>(peek()))) {
            parse_number();
            return;
        }
        throw std::runtime_error("UNEXPECTED_TOKEN");
    }

    bool match_str(const char* t) {
        std::size_t len = std::strlen(t);
        if (pos_ + len > s_.size()) return false;
        if (s_.compare(pos_, len, t) != 0) return false;
        pos_ += len;
        return true;
    }

    void parse_string() {
        if (get() != '"') return;
        if (peek() == '"') {
            get();
            hit(B_JSON_STRING_EMPTY);
            return;
        }
        hit(B_JSON_STRING_NORMAL);
        while (!eof() && peek() != '"') {
            if (peek() == '\\') {
                get();
                if (eof()) { hit(B_JSON_UNCLOSED_STRING); throw std::runtime_error("UNCLOSED_STRING"); }
                char e = get();
                if (e == '"') hit(B_JSON_ESCAPE_QUOTE);
                else if (e == '\\') hit(B_JSON_ESCAPE_BACKSLASH);
                else if (e == 'n') hit(B_JSON_ESCAPE_NEWLINE);
                else if (e == 'u') {
                    hit(B_JSON_UNICODE_ESCAPE);
                    for (int i = 0; i < 4; ++i) {
                        if (eof() || !std::isxdigit(static_cast<unsigned char>(peek()))) {
                            hit(B_JSON_INVALID_ESCAPE);
                            throw std::runtime_error("INVALID_ESCAPE");
                        }
                        get();
                    }
                } else {
                    hit(B_JSON_INVALID_ESCAPE);
                    throw std::runtime_error("INVALID_ESCAPE");
                }
            } else {
                get();
            }
        }
        if (!match('"')) {
            hit(B_JSON_UNCLOSED_STRING);
            throw std::runtime_error("UNCLOSED_STRING");
        }
    }

    void parse_number() {
        std::size_t start = pos_;
        bool neg = false;
        if (peek() == '-') { neg = true; get(); hit(B_JSON_NEGATIVE_NUM); }
        if (peek() == '0') {
            get();
            if (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                hit(B_JSON_NUMBER_LEADING_ZERO);
                throw std::runtime_error("NUMBER_LEADING_ZERO");
            }
            hit(B_JSON_ZERO);
            hit(B_JSON_VALUE_ZERO_INT);
        } else {
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) get();
            hit(B_JSON_INT);
        }
        bool is_float = false;
        if (peek() == '.') {
            is_float = true;
            get();
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) get();
            hit(B_JSON_FLOAT);
        }
        if (peek() == 'e' || peek() == 'E') {
            get();
            if (peek() == '+' || peek() == '-') get();
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) get();
            hit(B_JSON_SCI_NUMBER);
        }
        std::string tok = s_.substr(start, pos_ - start);
        try {
            long long v = std::stoll(tok);
            if (v > 9999) hit(B_JSON_VALUE_LARGE_INT);
            if (v < 0) hit(B_JSON_VALUE_NEG_INT);
        } catch (...) {}
        (void)is_float;
        (void)neg;
    }

    void parse_array() {
        if (get() != '[') return;
        depth_++;
        check_depth();
        skip_ws();
        if (peek() == ']') {
            get();
            hit(B_JSON_ARRAY_EMPTY);
            depth_--;
            return;
        }
        int count = 0;
        bool mixed = false;
        char last_type = 0;
        while (true) {
            char t = peek();
            if (t == '{') { hit(B_JSON_ARRAY_NESTED_OBJ); hit(B_JSON_NESTED_OBJECT); }
            if (t == '[') hit(B_JSON_NESTED_ARRAY);
            parse_value(1);
            char cur = 'v';
            if (t == 'n' || t == 't' || t == 'f') cur = 'p';
            if (last_type && last_type != cur) mixed = true;
            last_type = cur;
            count++;
            skip_ws();
            if (peek() == ']') { get(); break; }
            if (peek() != ',') {
                hit(B_JSON_UNCLOSED_ARRAY);
                throw std::runtime_error("UNCLOSED_ARRAY");
            }
            get();
            skip_ws();
            if (peek() == ']') {
                hit(B_JSON_TRAILING_COMMA);
                hit(B_JSON_EXTRA_COMMA);
                throw std::runtime_error("TRAILING_COMMA");
            }
        }
        if (count == 1) hit(B_JSON_ARRAY_ONE);
        if (count > 1) hit(B_JSON_ARRAY_MULTI);
        if (mixed) hit(B_JSON_MIXED_TYPES_ARRAY);
        depth_--;
    }

    void parse_object() {
        if (get() != '{') return;
        depth_++;
        check_depth();
        keys_.clear();
        skip_ws();
        if (peek() == '}') {
            get();
            hit(B_JSON_OBJECT_EMPTY);
            depth_--;
            return;
        }
        int pairs = 0;
        while (true) {
            skip_ws();
            if (peek() != '"') {
                hit(B_JSON_MISSING_COLON);
                throw std::runtime_error("MISSING_COLON");
            }
            std::size_t kstart = pos_ + 1;
            parse_string();
            std::string key = s_.substr(kstart, pos_ - kstart - 1);
            if (key.size() > 10) hit(B_JSON_KEY_LONG);
            bool all_digit = !key.empty();
            for (char c : key) {
                if (!std::isdigit(static_cast<unsigned char>(c))) all_digit = false;
            }
            if (all_digit && !key.empty()) hit(B_JSON_KEY_NUMERIC);
            if (keys_.count(key)) hit(B_JSON_KEY_COLLISION);
            keys_.insert(key);
            skip_ws();
            if (peek() != ':') {
                hit(B_JSON_MISSING_COLON);
                throw std::runtime_error("MISSING_COLON");
            }
            get();
            skip_ws();
            if (peek() == '[') hit(B_JSON_OBJECT_ARRAY_VAL);
            parse_value(0);
            pairs++;
            skip_ws();
            if (peek() == '}') { get(); break; }
            if (peek() != ',') {
                hit(B_JSON_UNCLOSED_OBJECT);
                throw std::runtime_error("UNCLOSED_OBJECT");
            }
            get();
        }
        if (pairs == 1) hit(B_JSON_OBJECT_ONE_PAIR);
        if (pairs > 1) hit(B_JSON_OBJECT_MULTI_PAIR);
        if (depth_ >= 2) hit(B_JSON_MIXED_NESTED);
        depth_--;
    }

    bool match(char c) {
        if (peek() == c) { get(); return true; }
        return false;
    }
};

} // namespace

std::size_t JsonSUT::branch_count() const noexcept { return kJsonBranches; }
std::string JsonSUT::name() const noexcept { return "json"; }

ExecutionResult JsonSUT::run(const std::string& input) const {
    ExecutionResult result;
    result.branch_count = branch_count();
    try {
        JsonParser p(input, result);
        p.parse();
    } catch (const std::exception& e) {
        result.crashed = true;
        result.crash_signature = e.what();
    }
    return result;
}
