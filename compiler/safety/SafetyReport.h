// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <cstdint>
#include <map>
#include <string>

class LocationManager;
struct LabModule;

namespace chem::safety {

/**
 * Counts collected by the read-only safety audit. The report never changes
 * compilation behavior; it exists to drive the report->migrate->enforce
 * workflow and to catch unreviewed growth.
 */
struct SafetyCounts {
    uint32_t raw_pointer_types = 0;
    uint32_t raw_pointer_derefs = 0;
    uint32_t raw_pointer_addrs = 0;
    uint32_t raw_pointer_casts = 0;
    uint32_t unsafe_blocks = 0;
    uint32_t unsafe_values = 0;
    uint32_t extern_decls = 0;
    uint32_t extern_calls = 0;
    uint32_t variadic_calls = 0;
    uint32_t panic_calls = 0;
    uint32_t abort_calls = 0;
    uint32_t unreachable_stmts = 0;
    uint32_t new_values = 0;
    uint32_t delete_stmts = 0;
    uint32_t dealloc_stmts = 0;
    uint32_t destruct_stmts = 0;
    uint32_t allocator_calls = 0;
    uint32_t memory_calls = 0;
    uint32_t inline_asm = 0;
    uint32_t mutable_globals = 0;
    uint32_t extern_globals = 0;
    uint32_t union_field_accesses = 0;

    void add(const SafetyCounts& other);
    [[nodiscard]] uint64_t total() const;
};

struct SafetyFileReport {
    std::string path;
    SafetyCounts counts;
};

struct SafetyReport {
    std::map<std::string, SafetyCounts> files;

    void add(const std::string& path, const SafetyCounts& counts);
    [[nodiscard]] SafetyCounts totals() const;
    void print_text(std::ostream& os) const;
    void print_json(std::ostream& os) const;
};

/**
 * Walk every direct file of a module and collect safety-relevant operations.
 * The module must have been parsed and linked (the report is intended for
 * --no-cache audits). Direct files without a parse result are skipped.
 */
void collect_module_report(LabModule& module, LocationManager& loc_man, SafetyReport& report);

} // namespace chem::safety
