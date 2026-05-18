// sut_url.cpp — embedded URL parser/validator (44 branches)
#include "sut_url.h"
#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

static constexpr std::size_t kUrlBranches = 44;

enum UrlBranch : std::size_t {
    B_URL_HTTP = 0, B_URL_HTTPS, B_URL_FTP, B_URL_FILE,
    B_URL_HOSTNAME, B_URL_IPV4, B_URL_LOCALHOST,
    B_URL_PORT_PRESENT, B_URL_PORT_80, B_URL_PORT_443, B_URL_PORT_LARGE,
    B_URL_PATH_EMPTY, B_URL_PATH_ROOT, B_URL_PATH_SINGLE_SEGMENT,
    B_URL_PATH_MULTI_SEGMENT, B_URL_PATH_DEPTH_GT5,
    B_URL_PATH_DOT_SEGMENT, B_URL_PATH_ENCODED,
    B_URL_QUERY_ABSENT, B_URL_QUERY_SINGLE_PARAM,
    B_URL_QUERY_MULTI_PARAM, B_URL_QUERY_NO_VALUE, B_URL_QUERY_ENCODED,
    B_URL_FRAGMENT_ABSENT, B_URL_FRAGMENT_PRESENT,
    B_URL_AUTHORITY_MISSING, B_URL_SCHEME_UNKNOWN,
    B_URL_DOUBLE_SLASH, B_URL_AT_SIGN, B_URL_EMPTY_LABEL,
    B_URL_LABEL_LONG, B_URL_IPV4_INVALID, B_URL_HOSTNAME_NUMERIC,
    B_URL_QUERY_INJECTION, B_URL_EMPTY_INPUT, B_URL_NO_SCHEME,
    B_URL_NO_AUTHORITY, B_URL_FRAGMENT_QUESTION,
    B_URL_PATH_ENCODED_INVALID, B_URL_MULTIPLE_AT,
    B_URL_QUERY_REPEATED_KEY, B_URL_PATH_SPACE, B_URL_AUTH_BRACKET
};

class UrlParser {
public:
    UrlParser(const std::string& s, ExecutionResult& res) : s_(s), res_(res) {
        res_.input_length = s.size();
    }

    void parse() {
        if (s_.empty()) {
            hit(B_URL_EMPTY_INPUT);
            throw std::runtime_error("EMPTY_INPUT");
        }
        std::size_t pos = 0;
        std::string scheme = parse_scheme(pos);
        if (scheme.empty()) {
            hit(B_URL_NO_SCHEME);
            throw std::runtime_error("NO_SCHEME");
        }
        if (scheme == "http") hit(B_URL_HTTP);
        else if (scheme == "https") hit(B_URL_HTTPS);
        else if (scheme == "ftp") hit(B_URL_FTP);
        else if (scheme == "file") hit(B_URL_FILE);
        else {
            hit(B_URL_SCHEME_UNKNOWN);
            throw std::runtime_error("UNKNOWN_SCHEME");
        }
        if (pos + 2 > s_.size() || s_[pos] != '/' || s_[pos + 1] != '/') {
            hit(B_URL_NO_AUTHORITY);
            throw std::runtime_error("NO_AUTHORITY");
        }
        pos += 2;
        std::size_t auth_end = s_.find_first_of("/?#", pos);
        if (auth_end == std::string::npos) auth_end = s_.size();
        std::string authority = s_.substr(pos, auth_end - pos);
        if (authority.empty()) {
            hit(B_URL_AUTHORITY_MISSING);
            throw std::runtime_error("AUTHORITY_MISSING");
        }
        int at_count = 0;
        for (char c : authority) if (c == '@') { at_count++; hit(B_URL_AT_SIGN); }
        if (at_count > 1) hit(B_URL_MULTIPLE_AT);
        if (authority.find('[') != std::string::npos) hit(B_URL_AUTH_BRACKET);
        parse_authority(authority);
        pos = auth_end;
        parse_path_query_fragment(pos);
    }

private:
    const std::string& s_;
    ExecutionResult& res_;

    void hit(std::size_t id) {
        if (id < kUrlBranches) res_.branch_hits[id] = true;
    }

    std::string parse_scheme(std::size_t& pos) {
        std::size_t colon = s_.find(':', pos);
        if (colon == std::string::npos) return {};
        std::string scheme = s_.substr(pos, colon - pos);
        pos = colon + 1;
        return scheme;
    }

    void parse_authority(const std::string& auth) {
        std::string hostport = auth;
        std::size_t at = auth.rfind('@');
        if (at != std::string::npos) hostport = auth.substr(at + 1);
        std::size_t colon = hostport.rfind(':');
        std::string host = hostport;
        std::string port_str;
        if (colon != std::string::npos) {
            host = hostport.substr(0, colon);
            port_str = hostport.substr(colon + 1);
            hit(B_URL_PORT_PRESENT);
            try {
                int port = std::stoi(port_str);
                if (port == 80) hit(B_URL_PORT_80);
                if (port == 443) hit(B_URL_PORT_443);
                if (port > 65535) {
                    hit(B_URL_PORT_LARGE);
                    throw std::runtime_error("PORT_OVERFLOW");
                }
            } catch (const std::invalid_argument&) {}
        }
        if (host == "localhost") hit(B_URL_LOCALHOST);
        if (host.find("..") != std::string::npos) hit(B_URL_EMPTY_LABEL);
        for (const auto& label : split(host, '.')) {
            if (label.empty()) {
                hit(B_URL_EMPTY_LABEL);
                throw std::runtime_error("EMPTY_LABEL");
            }
            if (label.size() > 63) hit(B_URL_LABEL_LONG);
        }
        if (looks_like_ipv4(host)) {
            hit(B_URL_IPV4);
            if (!valid_ipv4(host)) {
                hit(B_URL_IPV4_INVALID);
                throw std::runtime_error("INVALID_IPV4");
            }
        } else {
            hit(B_URL_HOSTNAME);
            bool all_num = !host.empty();
            for (const auto& label : split(host, '.')) {
                for (char c : label) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) all_num = false;
                }
            }
            if (all_num) hit(B_URL_HOSTNAME_NUMERIC);
        }
    }

    void parse_path_query_fragment(std::size_t pos) {
        std::size_t q = s_.find('?', pos);
        std::size_t f = s_.find('#', pos);
        std::size_t path_end = s_.size();
        if (q != std::string::npos) path_end = std::min(path_end, q);
        if (f != std::string::npos) path_end = std::min(path_end, f);
        std::string path = s_.substr(pos, path_end - pos);
        if (path.empty()) hit(B_URL_PATH_EMPTY);
        else if (path == "/") hit(B_URL_PATH_ROOT);
        else {
            auto segs = split(path, '/');
            int non_empty = 0;
            for (const auto& seg : segs) {
                if (seg.empty()) continue;
                non_empty++;
                if (seg == "..") {
                    hit(B_URL_PATH_DOT_SEGMENT);
                    throw std::runtime_error("DOTDOT_SEGMENT");
                }
                if (seg.find('%') != std::string::npos) {
                    hit(B_URL_PATH_ENCODED);
                    if (!valid_percent(seg)) {
                        hit(B_URL_PATH_ENCODED_INVALID);
                        throw std::runtime_error("INVALID_PERCENT_ENCODING");
                    }
                }
                if (seg.find(' ') != std::string::npos) hit(B_URL_PATH_SPACE);
            }
            if (path.find("//") != std::string::npos) hit(B_URL_DOUBLE_SLASH);
            if (non_empty == 1) hit(B_URL_PATH_SINGLE_SEGMENT);
            if (non_empty > 1) hit(B_URL_PATH_MULTI_SEGMENT);
            if (non_empty > 5) hit(B_URL_PATH_DEPTH_GT5);
        }
        if (q == std::string::npos) {
            hit(B_URL_QUERY_ABSENT);
        } else {
            std::size_t qend = (f != std::string::npos) ? f : s_.size();
            std::string query = s_.substr(q + 1, qend - q - 1);
            if (query.find(';') != std::string::npos) hit(B_URL_QUERY_INJECTION);
            auto params = split(query, '&');
            if (params.size() == 1) hit(B_URL_QUERY_SINGLE_PARAM);
            if (params.size() > 1) hit(B_URL_QUERY_MULTI_PARAM);
            std::unordered_map<std::string, int> keys;
            for (const auto& p : params) {
                std::size_t eq = p.find('=');
                std::string key = (eq == std::string::npos) ? p : p.substr(0, eq);
                if (eq == std::string::npos) hit(B_URL_QUERY_NO_VALUE);
                if (p.find('%') != std::string::npos) hit(B_URL_QUERY_ENCODED);
                keys[key]++;
                if (keys[key] > 1) hit(B_URL_QUERY_REPEATED_KEY);
            }
        }
        if (f == std::string::npos) {
            hit(B_URL_FRAGMENT_ABSENT);
        } else {
            hit(B_URL_FRAGMENT_PRESENT);
            std::string frag = s_.substr(f + 1);
            if (frag.find('?') != std::string::npos) hit(B_URL_FRAGMENT_QUESTION);
        }
    }

    static std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> out;
        std::size_t start = 0;
        for (std::size_t i = 0; i <= s.size(); ++i) {
            if (i == s.size() || s[i] == delim) {
                out.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        }
        return out;
    }

    static bool looks_like_ipv4(const std::string& host) {
        int dots = 0;
        for (char c : host) if (c == '.') dots++;
        return dots == 3;
    }

    static bool valid_ipv4(const std::string& host) {
        auto parts = split(host, '.');
        if (parts.size() != 4) return false;
        for (const auto& p : parts) {
            if (p.empty()) return false;
            int v = std::stoi(p);
            if (v < 0 || v > 255) return false;
        }
        return true;
    }

    static bool valid_percent(const std::string& seg) {
        for (std::size_t i = 0; i < seg.size(); ++i) {
            if (seg[i] == '%') {
                if (i + 2 >= seg.size()) return false;
                if (!std::isxdigit(static_cast<unsigned char>(seg[i + 1])) ||
                    !std::isxdigit(static_cast<unsigned char>(seg[i + 2]))) {
                    return false;
                }
            }
        }
        return true;
    }
};

} // namespace

std::size_t UrlSUT::branch_count() const noexcept { return kUrlBranches; }
std::string UrlSUT::name() const noexcept { return "url"; }

ExecutionResult UrlSUT::run(const std::string& input) const {
    ExecutionResult result;
    result.branch_count = branch_count();
    try {
        UrlParser p(input, result);
        p.parse();
    } catch (const std::exception& e) {
        result.crashed = true;
        result.crash_signature = e.what();
    }
    return result;
}
