// Copyright (c) Chemical Language Foundation 2025.

#include "LspTests.h"
#include <iostream>
#include <vector>
#include <string>
#include "lexer/Lexer.h"
#include "stream/InputSource.h"
#include "ast/base/BatchAllocator.h"
#include "compiler/cbi/model/CompilerBinder.h"
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
    CompilerBinder binder;
    Lexer lexer("test.ch", input, &binder, allocator);
    lexer.lex_whitespace = true;
    lexer.keep_comments = true;
    
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

void test_functions_and_parameters() {
    std::string input = "func add(a:int,b:int):int{return a+b}";
    std::string expected = 
"func add(a: int, b: int): int {\n"
"    return a + b\n"
"}";
    assert_equal("Functions and Parameters", expected, format_code(input));
}

void test_nested_structures() {
    std::string input = "namespace Math{struct Point{var x:float;var y:float}}";
    std::string expected = 
"namespace Math {\n"
"    struct Point {\n"
"        var x: float;\n"
"        var y: float\n"
"    }\n"
"}";
    assert_equal("Nested Structures", expected, format_code(input));
}

void test_control_flow_complex() {
    std::string input = 
"if(x>0){print(\"pos\")}else if(x<0){print(\"neg\")}else{print(\"zero\")}";
    
    std::string expected = 
"if (x > 0) {\n"
"    print(\"pos\")\n"
"} else if (x < 0) {\n"
"    print(\"neg\")\n"
"} else {\n"
"    print(\"zero\")\n"
"}";
    assert_equal("Complex Control Flow", expected, format_code(input));
}

void test_expression_complex() {
    std::string input = "var res=(a+b)*(c/d)%e";
    std::string expected = "var res = (a + b) * (c / d) % e";
    assert_equal("Complex Expressions", expected, format_code(input));
}

void test_comments_preservation() {
    std::string input = 
"// This is a comment\n"
"var x = 1 // end of line\n"
"/* Multi-line\n"
"   comment */\n"
"func main() {}";
    
    std::string expected = 
"// This is a comment\n"
"var x = 1 // end of line\n"
"/* Multi-line\n"
"   comment */\n"
"func main() {}";
    assert_equal("Comments Preservation", expected, format_code(input));
}

void test_arrays_and_indexing() {
    std::string input = "var arr:[5]int=[1,2,3,4,5];var x=arr[0]";
    std::string expected = 
"var arr: [5]int = [1, 2, 3, 4, 5];\n"
"var x = arr[0]";
    assert_equal("Arrays and Indexing", expected, format_code(input));
}

void test_annotations_and_macros() {
    std::string input = "@test\nfunc main(){}";
    std::string expected = 
"@test\n"
"func main() {}";
    // Note: our current logic adds a space after @annotation and #macro 
    // unless followed by newline. If it is already followed by newline, 
    // it will have a space before the newline if we aren't careful.
    // Let's see how it behaves.
    assert_equal("Annotations and Macros", expected, format_code(input));
}

void test_vertical_spacing() {
    std::string input = "var a = 1\n\n\n\nvar b = 2";
    std::string expected = "var a = 1\n\nvar b = 2";
    assert_equal("Vertical Spacing Decoration", expected, format_code(input));
}

} // namespace


void run_lsp_tests() {
    std::cout << "--- Running Formatter Tests ---" << std::endl;
    
    test_statement_merging_fix();
    test_basic_spacing();
    test_indentation();
    test_colon_spacing();
    test_functions_and_parameters();
    test_nested_structures();
    test_control_flow_complex();
    test_expression_complex();
    test_comments_preservation();
    test_arrays_and_indexing();
    test_annotations_and_macros();
    test_vertical_spacing();
    
    std::cout << "--- Formatter Tests Complete ---" << std::endl;
}

#endif
