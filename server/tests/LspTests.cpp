// Copyright (c) Chemical Language Foundation 2025.

#include "LspTests.h"
#include <iostream>
#include <vector>
#include <string>
#include "lexer/Lexer.h"
#include "stream/InputSource.h"
#include "ast/base/BatchAllocator.h"
#include "core/diag/Diagnoser.h"
#include "server/analyzers/FormatterAnalyzer.h"

#ifdef DEBUG

namespace {

struct TestResult {
    bool success;
    std::string message;
};

std::string format_code(const std::string& code) {
    Diagnoser diagnoser;
    InputSource input(code.data(), code.size());
    BatchAllocator allocator(4096);
    Lexer lexer("test.ch", input, nullptr, allocator);
    lexer.lex_whitespace = true;
    
    std::vector<Token> tokens;
    Token t;
    while ((t = lexer.getNextToken()).type != TokenType::EndOfFile) {
        tokens.push_back(t);
    }
    
    FormatterAnalyzer formatter;
    auto edits = formatter.format(tokens, code);
    
    if (edits.empty()) return code;
    
    // For our tests, we assume a single full-file replacement edit
    return edits[0].newText;
}

void assert_equal(const std::string& name, const std::string& expected, const std::string& actual) {
    if (expected == actual) {
        std::cout << "[PASS] " << name << std::endl;
    } else {
        std::cout << "[FAIL] " << name << std::endl;
        std::cout << "  Expected: " << std::endl << "\"" << expected << "\"" << std::endl;
        std::cout << "  Actual:   " << std::endl << "\"" << actual << "\"" << std::endl;
        
        // Find first difference for easier debugging
        size_t len = std::min(expected.length(), actual.length());
        for(size_t i = 0; i < len; ++i) {
            if(expected[i] != actual[i]) {
                std::cout << "  First diff at index " << i << ": expected '" << expected[i] << "', got '" << actual[i] << "'" << std::endl;
                break;
            }
        }
    }
}

void test_statement_merging_fix() {
    std::string input = 
"struct MyPoint {\n"
"    var a: int\n"
"    var b: int\n"
"}";
    
    // We expect the formatting to preserve the newlines
    std::string expected = 
"struct MyPoint {\n"
"    var a: int\n"
"    var b: int\n"
"}";
    
    assert_equal("Statement Merging Fix", expected, format_code(input));
}

void test_basic_spacing() {
    std::string input = "var x=1+2";
    std::string expected = "var x = 1 + 2";
    assert_equal("Basic Spacing", expected, format_code(input));
}

void test_indentation() {
    std::string input = 
"func main() {\n"
"if(true) {\n"
"var x = 1\n"
"}\n"
"}";
    
    std::string expected = 
"func main() {\n"
"    if (true) {\n"
"        var x = 1\n"
"    }\n"
"}";
    
    assert_equal("Indentation", expected, format_code(input));
}

void test_colon_spacing() {
    std::string input = "var a:int = 5";
    std::string expected = "var a: int = 5";
    assert_equal("Colon Spacing", expected, format_code(input));
}

} // namespace

void run_lsp_tests() {
    std::cout << "--- Running Formatter Tests ---" << std::endl;
    
    test_statement_merging_fix();
    test_basic_spacing();
    test_indentation();
    test_colon_spacing();
    
    std::cout << "--- Formatter Tests Complete ---" << std::endl;
}

#endif
