// Copyright (c) Chemical Language Foundation 2026.
//
// MIR-to-JavaScript emitter.
//
// The backend is a *thin printer over MIR* (see lang/docs/mir-design.md §16):
// types, layout, mangling and constants all come from MIR; nothing here reads
// the AST. The runtime it targets is `scripts/js_backend/runtime.mjs`, which
// owns the data model (emulated linear memory, typed loads/stores, C integer
// arithmetic), so generated code never touches a DataView.
//
// Design and rationale: lang/docs/js-backend-abi.md (Route A, emulated linear
// memory) and lang/docs/web-stack-plan.md §3.9 (the JS backend).
//
// One Chemical module becomes one ES module. Functions are emitted as
// `export function $f<symbol>(...)`, values as `v<id>` and places as `p<id>`,
// exactly mirroring the MIR value/place tables.

#pragma once

#include "MIRModule.h"

#include <string>
#include <string_view>
#include <vector>

namespace mir {

struct MIREmitJsOptions {
    /** The import specifier the emitted module uses for the runtime. */
    std::string runtime_specifier = "./runtime.mjs";
    /** The Chemical module's name, for the file header comment. */
    std::string module_name;
    /**
     * Emit `// line: instruction` comments. Off by default: the MIR dump
     * (`MIR_C_DUMP`) is the debugging tool, not the generated JS.
     */
    bool instruction_comments = false;
    /**
     * Write compact JavaScript: comments, indentation and every whitespace run
     * that no token pair needs are removed. This is a *formatting* pass only —
     * identifiers keep their names, so a stack trace from a release bundle still
     * reads like the MIR (`$f22`, `$g27`) instead of `a`, `b`, `c`.
     *
     * No source map is produced with it: line and column numbers no longer
     * correspond, and a map that points at the wrong line is worse than none.
     */
    bool minify = false;
    /**
     * Rename each function's values and places to short names (`a`, `b`, …) in a
     * release build. MIR ids are an implementation detail of this emitter, so a
     * shipped bundle should not carry them; exported functions keep `$f<id>`
     * because another module imports them by that name.
     */
    bool mangle_names = false;
};

/**
 * Strip comments and formatting from emitted JavaScript. String and template
 * literals are copied byte for byte, and a single space is kept wherever two
 * tokens would otherwise merge (`return x`, `a + +b`).
 */
std::string minify_js_source(const std::string& source);

/**
 * Emit `module` as one ES module. On failure returns false and sets `error`
 * (with the function and instruction index); `out` is then meaningless.
 *
 * Unsupported constructs are *diagnosed*, never emitted half-working:
 * coroutines (`Async*`), atomics, inline assembly, interface dispatch
 * (`DynamicCall`) and calls to functions defined in another Chemical module
 * (cross-module imports land with the module split).
 */
bool emit_module_js(const MIRModule& module, std::string& out, std::string& error,
                    const MIREmitJsOptions& options = MIREmitJsOptions{},
                    std::string* source_map = nullptr);

/** One emitted file of a split bundle. */
struct JsEmittedModule {
    /** File stem (`app__main`); the file is `<stem>.mjs`, the map `<stem>.mjs.map`. */
    std::string stem;
    /** The Chemical module this file represents (empty for an unattributed symbol). */
    std::string module_name;
    std::string source;
    std::string source_map;
};

/**
 * Emit one ES module **per Chemical module** that defined a reachable function,
 * with imports between them.
 *
 * This is what lets a browser cache a module's code separately and what makes an
 * emitted bundle a real ESM graph rather than one concatenated file. Ownership
 * comes from `MIRModule::module_of_symbol` (filled at lowering); a symbol whose
 * owner is unknown is emitted into the job's own module rather than dropped, so
 * a partially-attributed program still builds.
 *
 * `files` receives one entry per module, in discovery order. The caller writes
 * them and an entry module that merges their `$exports` tables.
 */
bool emit_modules_js(const MIRModule& module, const std::vector<const MIRFunction*>& functions,
                     std::vector<JsEmittedModule>& files, std::string& error,
                     const MIREmitJsOptions& options);

/**
 * Emit `functions` (a subset of `module.functions`) as one ES module. The module
 * tables are shared, so cross-references between the emitted functions resolve
 * as direct calls.
 *
 * When `source_map` is non-null it receives a version-3 source map for the
 * module, and the module gets a trailing `sourceMappingURL` comment.
 */
bool emit_program_js(const MIRModule& module, const std::vector<const MIRFunction*>& functions,
                     std::string& out, std::string& error,
                     const MIREmitJsOptions& options = MIREmitJsOptions{},
                     std::string* source_map = nullptr);

/**
 * The functions reachable from `entry` (a mangled symbol name) by walking call
 * and function-address references, in discovery order.
 *
 * This is what makes the backend usable on a program that links platform code it
 * never calls (every `cstd` module has thousands of unused externs), and it is
 * also the whole-program dead-code elimination the bundle needs.
 */
std::vector<const MIRFunction*> reachable_functions(const MIRModule& module, std::string_view entry);

/** The JS spelling of a function symbol: `$f12`. */
std::string js_function_name(SymbolId symbol);

/** `app::main` -> `app__main`, a safe file stem for the emitted module. */
std::string js_module_file_stem(const std::string& module_name);

} // namespace mir
