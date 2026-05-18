// experiment.cpp — grammars and experiment orchestration
#include "experiment.h"
#include "sut.h"
#include "sut_csv.h"
#include "sut_json.h"
#include "sut_url.h"
#include <memory>
#include <stdexcept>
#include <utility>

Grammar build_rich_grammar() {
    Grammar g;
    g.start_symbol = "<S>";

    // Start symbol
    g.productions["<S>"] = {
        {"<E>"}
    };

    // Expression with ternary, assignment-level complexity
    g.productions["<E>"] = {
        {"<T>"},
        {"<T>", "+", "<E>"},
        {"<T>", "-", "<E>"},
        {"<T>", "?", "<E>", ":", "<E>"}  // ternary operator
    };

    // Term with comparison and logical operators
    g.productions["<T>"] = {
        {"<R>"},
        {"<R>", "*", "<T>"},
        {"<R>", "/", "<T>"},
        {"<R>", "%", "<T>"},
        {"<R>", "<", "<T>"},
        {"<R>", ">", "<T>"},
        {"<R>", "==", "<T>"},
        {"<R>", "!=", "<T>"},
        {"<R>", "&&", "<T>"},
        {"<R>", "||", "<T>"}
    };

    // Relational/Additive (used to create depth)
    g.productions["<R>"] = {
        {"<F>"},
        {"<U>", "<R>"}  // unary operators
    };

    // Factor: primary expressions
    g.productions["<F>"] = {
        {"<NUM>"},
        {"(", "<E>", ")"},
        {"<FUNC>"},
        {"<VAR>"}
    };

    // Unary operators
    g.productions["<U>"] = {
        {"+"},
        {"-"},
        {"!"},
        {"~"}
    };

    // Functions
    g.productions["<FUNC>"] = {
        {"sin(", "<E>", ")"},
        {"cos(", "<E>", ")"},
        {"abs(", "<E>", ")"},
        {"sqrt(", "<E>", ")"}
    };

    // Variables
    g.productions["<VAR>"] = {
        {"x"},
        {"y"},
        {"z"}
    };

    // Numbers: integer, float, hex, scientific
    g.productions["<NUM>"] = {
        {"<INT>"},
        {"<FLOAT>"},
        {"<HEX>"},
        {"<SCI>"}
    };

    // Integer
    g.productions["<INT>"] = {
        {"<DIGIT>"},
        {"<DIGIT>", "<INT>"}
    };

    // Float: digits.digits
    g.productions["<FLOAT>"] = {
        {"<INT>", ".", "<INT>"}
    };

    // Hex: 0xhexdigits
    g.productions["<HEX>"] = {
        {"0x", "<HEXDIG>"},
        {"0x", "<HEXDIG>", "<HEXDIG>"},
        {"0x", "<HEXDIG>", "<HEXDIG>", "<HEXDIG>"}
    };

    // Scientific: digits e digits
    g.productions["<SCI>"] = {
        {"<INT>", "e", "<INT>"},
        {"<INT>", "e", "+", "<INT>"},
        {"<INT>", "e", "-", "<INT>"}
    };

    // Digits
    g.productions["<DIGIT>"] = {
        {"0"}, {"1"}, {"2"}, {"3"}, {"4"},
        {"5"}, {"6"}, {"7"}, {"8"}, {"9"}
    };

    // Hex digits
    g.productions["<HEXDIG>"] = {
        {"0"}, {"1"}, {"2"}, {"3"}, {"4"},
        {"5"}, {"6"}, {"7"}, {"8"}, {"9"},
        {"a"}, {"b"}, {"c"}, {"d"}, {"e"}, {"f"}
    };

    return g;
}

Grammar build_json_grammar() {
    Grammar g;
    g.start_symbol = "<start>";
    g.productions["<start>"] = {{"<value>"}};
    g.productions["<value>"] = {
        {"<object>"}, {"<array>"}, {"<string>"}, {"<number>"},
        {"true"}, {"false"}, {"null"}
    };
    g.productions["<object>"] = {{"{}"}, {"{", "<members>", "}"}};
    g.productions["<members>"] = {{"<pair>"}, {"<pair>", ",", "<members>"}};
    g.productions["<pair>"] = {{"<string>", ":", "<value>"}};
    g.productions["<array>"] = {{"[]"}, {"[", "<elements>", "]"}};
    g.productions["<elements>"] = {{"<value>"}, {"<value>", ",", "<elements>"}};
    g.productions["<string>"] = {{"\"", "<chars>", "\""}};
    g.productions["<chars>"] = {{}, {"<char>", "<chars>"}};
    g.productions["<char>"] = {
        {"a"}, {"b"}, {"0"}, {"1"},
        {"\\", "<escape>"}
    };
    g.productions["<escape>"] = {{"\""}, {"\\"}, {"n"}, {"t"}, {"r"}};
    g.productions["<number>"] = {{"<int>"}, {"<int>", ".", "<digits>"}, {"<int>", "e", "<digits>"}};
    g.productions["<int>"] = {{"<digit>"}, {"-", "<digits>"}, {"<digits>"}};
    g.productions["<digit>"] = {{"0"}, {"1"}, {"2"}, {"3"}, {"4"}, {"5"}, {"6"}, {"7"}, {"8"}, {"9"}};
    g.productions["<digits>"] = {{"<digit>"}, {"<digit>", "<digits>"}};
    return g;
}

Grammar build_csv_grammar() {
    Grammar g;
    g.start_symbol = "<start>";
    g.productions["<start>"] = {{"<rows>"}};
    g.productions["<rows>"] = {{"<row>"}, {"<row>", "\\n", "<rows>"}};
    g.productions["<row>"] = {{"<fields>"}};
    g.productions["<fields>"] = {{"<field>"}, {"<field>", ",", "<fields>"}};
    g.productions["<field>"] = {{"<quoted>"}, {"<unquoted>"}};
    g.productions["<quoted>"] = {{"\"", "<qchars>", "\""}};
    g.productions["<qchars>"] = {{}, {"<qchar>", "<qchars>"}};
    g.productions["<qchar>"] = {{"a"}, {"b"}, {"\"\""}};
    g.productions["<unquoted>"] = {{"<word>"}, {""}};
    g.productions["<word>"] = {{"<wchar>"}, {"<wchar>", "<word>"}};
    g.productions["<wchar>"] = {{"a"}, {"A"}, {"0"}, {"_"}, {"-"}, {"."}};
    return g;
}

Grammar build_url_grammar() {
    Grammar g;
    g.start_symbol = "<start>";
    g.productions["<start>"] = {{"<url>"}};
    g.productions["<url>"] = {{"<scheme>", "://", "<authority>", "<path>", "<query>", "<fragment>"}};
    g.productions["<scheme>"] = {{"http"}, {"https"}, {"ftp"}, {"file"}};
    g.productions["<authority>"] = {{"<host>"}, {"<host>", ":", "<port>"}};
    g.productions["<host>"] = {{"<hostname>"}, {"<ipv4>"}};
    g.productions["<hostname>"] = {{"<label>"}, {"<label>", ".", "<hostname>"}};
    g.productions["<label>"] = {{"<word>"}};
    g.productions["<ipv4>"] = {{"<octet>", ".", "<octet>", ".", "<octet>", ".", "<octet>"}};
    g.productions["<octet>"] = {{"<digit>"}, {"<digit>", "<digit>"}};
    g.productions["<port>"] = {{"<digits>"}};
    g.productions["<path>"] = {{}, {"/"}, {"/", "<segments>"}};
    g.productions["<segments>"] = {{"<segment>"}, {"<segment>", "/", "<segments>"}};
    g.productions["<segment>"] = {{"<pchars>"}};
    g.productions["<pchars>"] = {{"<pchar>"}, {"<pchar>", "<pchars>"}};
    g.productions["<pchar>"] = {{"a"}, {"0"}, {"-"}, {"%"}, {"<hex>", "<hex>"}};
    g.productions["<query>"] = {{}, {"?", "<qparams>"}};
    g.productions["<qparams>"] = {{"<qparam>"}, {"<qparam>", "&", "<qparams>"}};
    g.productions["<qparam>"] = {{"<key>", "=", "<qvalue>"}, {"<key>"}};
    g.productions["<key>"] = {{"<word>"}};
    g.productions["<qvalue>"] = {{"<word>"}, {""}};
    g.productions["<fragment>"] = {{}, {"#", "<word>"}};
    g.productions["<word>"] = {{"<wchar>"}, {"<wchar>", "<word>"}};
    g.productions["<wchar>"] = {{"a"}, {"0"}, {"-"}, {"_"}};
    g.productions["<hex>"] = {{"0"}, {"a"}, {"f"}};
    g.productions["<digit>"] = {{"0"}, {"1"}, {"2"}, {"3"}, {"4"}, {"5"}, {"6"}, {"7"}, {"8"}, {"9"}};
    g.productions["<digits>"] = {{"<digit>"}, {"<digit>", "<digits>"}};
    return g;
}

Grammar grammar_for_sut(const std::string& sut_name) {
    if (sut_name == "arithmetic") return build_rich_grammar();
    if (sut_name == "json") return build_json_grammar();
    if (sut_name == "csv") return build_csv_grammar();
    if (sut_name == "url") return build_url_grammar();
    throw std::invalid_argument("Unknown SUT for grammar: " + sut_name);
}

std::unique_ptr<ISut> make_sut(const std::string& name) {
    if (name == "arithmetic") return std::make_unique<MiniSUT>();
    if (name == "json") return std::make_unique<JsonSUT>();
    if (name == "csv") return std::make_unique<CsvSUT>();
    if (name == "url") return std::make_unique<UrlSUT>();
    throw std::invalid_argument("Unknown SUT: " + name);
}

void run_experiments(
    const Grammar& grammar,
    const GAConfig& cfg,
    const ISut& sut,
    int runs,
    std::vector<RunSummary>& baseline_runs,
    std::vector<RunSummary>& adaptive_runs,
    std::vector<GenerationRecord>& generation_logs
) {
    baseline_runs.clear();
    adaptive_runs.clear();
    generation_logs.clear();

    baseline_runs.reserve(runs);
    adaptive_runs.reserve(runs);

    for (int run = 0; run < runs; ++run) {
        GAConfig local_cfg = cfg;
        local_cfg.seed = cfg.seed + static_cast<std::uint32_t>(run);

        baseline_runs.push_back(run_baseline_experiment(grammar, local_cfg, sut, run));

        AdaptiveResult adaptive = run_adaptive_experiment(grammar, local_cfg, sut, run);
        adaptive_runs.push_back(adaptive.summary);

        for (const auto& rec : adaptive.generation_log) {
            generation_logs.push_back(rec);
        }
    }
}
