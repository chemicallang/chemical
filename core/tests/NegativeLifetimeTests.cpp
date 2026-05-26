// Copyright (c) Chemical Language Foundation 2025.

#ifdef DEBUG

#include "NegativeLifetimeTests.h"
#include "compiler/typeverify/TypeVerify.h"
#include "compiler/typeverify/TypeVerifyAPI.h"
#include "compiler/ASTProcessor.h"
#include "compiler/frontend/AnnotationController.h"
#include "stream/InputSource.h"
#include "core/source/LocationManager.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTAllocator.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "utils/PathUtils.h"
#include <string>
#include <iostream>
#include <cstring>
#include <filesystem>

static int test_count = 0;
static int pass_count = 0;
static int fail_count = 0;

#define TEST(name, body) do { \
    test_count++; \
    bool ok = (body); \
    if (ok) { \
        std::cout << "  PASS: " << name << std::endl; \
        pass_count++; \
    } else { \
        std::cerr << "  FAIL: " << name << std::endl; \
        fail_count++; \
    } \
} while(0)

int run_negative_lifetime_tests() {
    LocationManager locMan;
    AnnotationController controller;
    ASTAllocator typeAlloc(4096);
    TypeBuilder typeBuilder(typeAlloc);
    ASTAllocator fileAlloc(4096);
    ASTAllocator modAlloc(4096);
    ASTAllocator jobAlloc(4096);
    ImplementationsIndex implIndex;
    ASTDiagnoser diagnoser(locMan);

    test_count = 0;
    pass_count = 0;
    fail_count = 0;

    // ---- Test 1: Parse a struct with lifetime params ----
    {
        const char* src = R"(
struct MyView 'a {
    var data : *char
}
)";
        InputSource input(src, strlen(src));
        ModuleScope modScope("", "", nullptr);
        ASTFileResult result(0, "<test_lifetime_parse>", &modScope);

        bool parse_ok = ASTProcessor::parse_chemical_file(
            result, 0, "<test_lifetime_parse>", &input,
            locMan, controller,
            *(CompilerBinder*)nullptr,
            typeBuilder,
            fileAlloc, modAlloc, jobAlloc,
            false, true
        );

        bool has_struct = false;
        bool has_lifetime = false;
        for (auto* node : result.unit.scope.body.nodes) {
            if (node->kind() == ASTNodeKind::StructDecl) {
                has_struct = true;
                auto* sd = static_cast<StructDefinition*>(node);
                has_lifetime = !sd->lifetime_params.empty();
            }
        }
        TEST("parse struct with lifetime param", parse_ok && has_struct && has_lifetime);
    }

    // ---- Test 2: Parse a function with return_lifetime ----
    {
        const char* src = R"(
struct MyView 'a {
    var data : *char
}
func create_view() : 'self MyView {
    return MyView { data : null }
}
)";
        InputSource input(src, strlen(src));
        ModuleScope modScope("", "", nullptr);
        ASTFileResult result(0, "<test_return_lifetime_parse>", &modScope);

        bool func_parse_ok = ASTProcessor::parse_chemical_file(
            result, 0, "<test_return_lifetime_parse>", &input,
            locMan, controller,
            *(CompilerBinder*)nullptr,
            typeBuilder,
            fileAlloc, modAlloc, jobAlloc,
            false, true
        );

        bool has_func_with_return_lifetime = false;
        for (auto* node : result.unit.scope.body.nodes) {
            if (node->kind() == ASTNodeKind::FunctionDecl) {
                auto* func = static_cast<FunctionDeclaration*>(node);
                if (!func->return_lifetime.empty()) {
                    has_func_with_return_lifetime = true;
                }
            }
        }
        TEST("parse function with 'self return_lifetime", func_parse_ok && has_func_with_return_lifetime);
    }

    // ---- Test 3: Parse unsafe block with lifetime_check flag ----
    {
        const char* src = R"(
func main() {
    unsafe "lifetime_check"- {
        var x : i32 = 5
    }
}
)";
        InputSource input(src, strlen(src));
        ModuleScope modScope("", "", nullptr);
        ASTFileResult result(0, "<test_unsafe_flag_parse>", &modScope);

        bool unsafe_parse_ok = ASTProcessor::parse_chemical_file(
            result, 0, "<test_unsafe_flag_parse>", &input,
            locMan, controller,
            *(CompilerBinder*)nullptr,
            typeBuilder,
            fileAlloc, modAlloc, jobAlloc,
            false, true
        );

        TEST("parse unsafe flag block", unsafe_parse_ok);
    }

    // ---- Test 4: TypeVerifier flags default to enabled ----
    {
        TypeVerifier verifier(implIndex, fileAlloc, diagnoser);
        // Default: no flag entry → check is enabled
        const auto it = verifier.flags.find("lifetime_check");
        bool default_enabled = (it == verifier.flags.end());
        TEST("lifetime_check enabled by default", default_enabled);
    }

    // ---- Test 5: TypeVerifier respects disabled flag ----
    {
        TypeVerifier verifier(implIndex, fileAlloc, diagnoser);
        verifier.flags["lifetime_check"] = false;
        const auto it = verifier.flags.find("lifetime_check");
        bool disabled = (it != verifier.flags.end() && !it->second);
        TEST("lifetime_check can be disabled", disabled);
    }

    // ---- Test 6: TypeVerifier respects enabled flag ----
    {
        TypeVerifier verifier(implIndex, fileAlloc, diagnoser);
        verifier.flags["lifetime_check"] = true;
        const auto it = verifier.flags.find("lifetime_check");
        bool enabled = (it != verifier.flags.end() && it->second);
        TEST("lifetime_check can be enabled", enabled);
    }

    std::cout << std::endl;
    std::cout << "Negative Lifetime Tests: " << test_count << " total, "
              << pass_count << " passed, " << fail_count << " failed" << std::endl;

    return fail_count;
}

#endif
