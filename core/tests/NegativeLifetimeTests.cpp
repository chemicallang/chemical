// Copyright (c) Chemical Language Foundation 2025.

#ifdef DEBUG

#include "NegativeLifetimeTests.h"
#include <string>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <array>
#include <cstring>
#include "utils/PathUtils.h"

static std::string work_dir;

static bool setup_work_dir() {
    auto path = std::filesystem::temp_directory_path();
    path /= "chemical_neg_tests";
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    std::filesystem::create_directories(path, ec);
    if (ec) return false;
    work_dir = path.string();
    return true;
}

struct NegTest {
    std::string name;
    std::string mod_source;   // chemical.mod content
    std::string ch_source;    // the .ch source file
    bool expect_error;        // true = compiler should error
    std::string expected_sub; // substring expected in output (only for expect_error=true)
};

static const NegTest tests[] = {
    {
        "temp_to_view_with_dtor_errors",
        // chemical.mod
        "module neg_test\nsource \".\"\n",
        // test.ch
        R"(struct MyView 'a {
    var data : *char
}
struct MyObj {
    func get_view(&self) : 'self MyView {
        return MyView { data : null }
    }
    @delete
    func delete(&mut self) { }
}
func main() {
    var v = MyObj().get_view()
}
)",
        true,   // expect compilation error
        "lifetime dependency"
    },
    {
        "named_to_view_succeeds",
        "module neg_test\nsource \".\"\n",
        R"(struct MyView 'a {
    var data : *char
}
struct MyObj {
    func get_view(&self) : 'self MyView {
        return MyView { data : null }
    }
    @delete
    func delete(&mut self) { }
}
func main() {
    var obj = MyObj()
    var v = obj.get_view()
}
)",
        false,  // expect compilation success
        ""
    },
    {
        "temp_no_lifetime_type_succeeds",
        "module neg_test\nsource \".\"\n",
        R"(struct MyObj {
    func get_value(&self) : i32 {
        return 42
    }
    @delete
    func delete(&mut self) { }
}
func main() {
    var v = MyObj().get_value()
}
)",
        false,
        ""
    },
    {
        "temp_no_dtor_succeeds",
        "module neg_test\nsource \".\"\n",
        R"(struct MyView 'a {
    var data : *char
}
struct MyObj {
    func get_view(&self) : 'self MyView {
        return MyView { data : null }
    }
}
func main() {
    var v = MyObj().get_view()
}
)",
        false,
        ""
    },
    {
        "temp_no_return_lifetime_succeeds",
        "module neg_test\nsource \".\"\n",
        R"(struct MyView 'a {
    var data : *char
}
func get_view(v : MyView) : MyView {
    return v
}
func main() {
    var v = get_view(MyView { data : null })
}
)",
        false,
        ""
    },
};

int run_negative_lifetime_tests() {
    if (!setup_work_dir()) {
        std::cerr << "FAILED to create temp directory for negative tests" << std::endl;
        return 1;
    }

    int passed = 0;
    int failed = 0;
    const auto exe = getExecutablePath();

    for (const auto& t : tests) {
        // Create test directory
        auto test_dir = work_dir + "/" + t.name;
        std::error_code ec;
        std::filesystem::create_directories(test_dir, ec);
        if (ec) {
            std::cerr << "FAILED to create " << test_dir << std::endl;
            failed++;
            continue;
        }

        // Write chemical.mod
        {
            FILE* f = fopen((test_dir + "/chemical.mod").c_str(), "w");
            if (f) {
                fwrite(t.mod_source.data(), 1, t.mod_source.size(), f);
                fclose(f);
            }
        }

        // Write test.ch
        {
            FILE* f = fopen((test_dir + "/test.ch").c_str(), "w");
            if (f) {
                fwrite(t.ch_source.data(), 1, t.ch_source.size(), f);
                fclose(f);
            }
        }

        // Compile
        std::string cmd = "\"" + exe + "\" \"" + test_dir + "/chemical.mod\" -o \"" + test_dir + "/out\" --no-cache 2>&1";
        std::array<char, 16384> buffer;
        std::string output;
        auto pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "  FAIL: " << t.name << " (popen failed)" << std::endl;
            failed++;
            continue;
        }
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            output += buffer.data();
        }
        int rc = pclose(pipe);
        bool compile_ok = (rc == 0);

        bool test_pass;
        if (t.expect_error) {
            // Expect compilation failure and specific substring
            bool has_error = output.find("[TypeCheck] error") != std::string::npos;
            bool has_sub = t.expected_sub.empty() || output.find(t.expected_sub) != std::string::npos;
            test_pass = !compile_ok && has_error && has_sub;
        } else {
            test_pass = compile_ok;
        }

        if (test_pass) {
            std::cout << "  PASS: " << t.name << std::endl;
            passed++;
        } else {
            std::cerr << "  FAIL: " << t.name << std::endl;
            std::cerr << "       compile_ok=" << compile_ok << " (expected=" << (t.expect_error ? "false" : "true") << ")" << std::endl;
            if (!output.empty()) {
                // Print last few lines of output
                size_t pos = output.size();
                for (int i = 0; i < 3 && pos != std::string::npos; i++) {
                    pos = output.rfind('\n', pos - 1);
                }
                if (pos == std::string::npos) pos = 0;
                std::cerr << "       output: " << output.substr(pos) << std::endl;
            }
            failed++;
        }

        // Cleanup
        std::filesystem::remove_all(test_dir, ec);
    }

    std::error_code ec;
    std::filesystem::remove_all(work_dir, ec);

    std::cout << std::endl;
    std::cout << "Negative Lifetime Tests: " << (passed + failed) << " total, "
              << passed << " passed, " << failed << " failed" << std::endl;

    return failed;
}

#endif
