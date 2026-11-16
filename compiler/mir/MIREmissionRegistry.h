// Copyright (c) Chemical Language Foundation 2025.
//
// Job-wide ownership of emitted *definitions*.
//
// Generic instantiation is per module: every module that mentions `Option<T>`
// instantiates its own copy, and a copy reached by two modules is two
// `StructDefinition` nodes with two sets of functions. Their runtime names are
// content derived (the generic declaration's qualified name plus its type
// arguments, see `instantiation_mangle_hash`), so both copies spell them
// identically — which is required, because module B calls the instantiation
// module A emitted. But a program can only *define* a name once: the C
// single-file build writes every module into one translation unit, and the
// per-module object build links them together.
//
// So the first module to reach a name owns its definition and every later copy
// is skipped. That is why this registry is job-wide and not per module: with a
// positional name (`__cgs__3`) the copies silently got different names, the
// duplicates linked as dead code, and two *different* types that happened to
// share a position overwrote each other.
//
// Two sets, because the two backends need different scopes:
//
//  - **types** (struct/union/variant bodies, typedefs, vtable types): a C
//    translation unit cannot define one twice, so the C single-file build
//    deduplicates them. LLVM does not: its types live inside each per-module
//    module, so every module must see the full type.
//  - **symbols** (function bodies, global definitions, vtable definitions):
//    both backends deduplicate. A C translation unit cannot define a function
//    twice, and two per-module objects cannot both define the same external
//    symbol.
//
// Thread safety: the declare pass of a module is serial, but function bodies
// are lowered by several workers in parallel, so every mutation takes the lock.
// A claim is one hash lookup on the hot path (once per declaration/body).

#pragma once

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace mir {

class MIREmissionRegistry {
public:
    /** Claim a type name for this job. False when a previous module owns it. */
    bool claim_type(std::string_view name) {
        return claim(types_, module_types_, skipped_types_, name);
    }

    /** Claim a symbol (function/global/vtable definition) for this job. */
    bool claim_symbol(std::string_view name) {
        return claim(symbols_, module_symbols_, skipped_symbols_, name);
    }

    /** True when some module of this job already emitted this type. */
    bool type_claimed(std::string_view name) {
        std::lock_guard<std::mutex> lk(mtx_);
        return types_.find(std::string(name)) != types_.end();
    }

    bool symbol_claimed(std::string_view name) {
        std::lock_guard<std::mutex> lk(mtx_);
        return symbols_.find(std::string(name)) != symbols_.end();
    }

    /** Number of definitions that were skipped because a module already owned
     *  them (a test/trace hook: a healthy build of a program with instantiated
     *  generics is non-zero, and it must never lose a *distinct* definition). */
    size_t skipped_types() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return skipped_types_;
    }
    size_t skipped_symbols() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return skipped_symbols_;
    }

    /**
     * Claim names that a module already emitted in an earlier build, recorded
     * next to its cached translation (see the lab driver's `inst.names`). Without
     * this a module served from cache owns nothing, and a module translated in
     * the same build would emit a second definition of every instantiation both
     * of them reach.
     */
    void load_type(std::string_view name) { claim_type(name); }
    void load_symbol(std::string_view name) { claim_symbol(name); }

    /** Start recording the names claimed by one module (see `take_module_names`). */
    void begin_module() {
        std::lock_guard<std::mutex> lk(mtx_);
        module_types_.clear();
        module_symbols_.clear();
    }

    /**
     * The names claimed since `begin_module`, in emission order. The C
     * single-file build persists them next to a module's cached translation so
     * a later build can claim them again without re-translating the module
     * (see `partial_names_path` in the lab driver).
     */
    void take_module_names(std::vector<std::string>& types, std::vector<std::string>& symbols) {
        std::lock_guard<std::mutex> lk(mtx_);
        types.swap(module_types_);
        symbols.swap(module_symbols_);
        module_types_.clear();
        module_symbols_.clear();
    }

private:
    bool claim(std::unordered_set<std::string>& set, std::vector<std::string>& journal,
               size_t& skipped, std::string_view name) {
        if (name.empty()) return true; // nothing to own (should not happen)
        std::lock_guard<std::mutex> lk(mtx_);
        std::string key(name);
        if (!set.insert(std::move(key)).second) {
            ++skipped;
            return false;
        }
        journal.emplace_back(name);
        return true;
    }

    mutable std::mutex mtx_;
    std::unordered_set<std::string> types_;
    std::unordered_set<std::string> symbols_;
    std::vector<std::string> module_types_;
    std::vector<std::string> module_symbols_;
    size_t skipped_types_ = 0;
    size_t skipped_symbols_ = 0;
};

} // namespace mir
