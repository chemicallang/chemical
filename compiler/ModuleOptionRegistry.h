// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>
#include <unordered_map>
#include <span>
#include "std/chem_string_view.h"
#include "core/source/SourceLocation.h"

// ---------------------------------------------------------------------------
// Parsed option value (stored as parsed from chemical.mod)
// ---------------------------------------------------------------------------

enum class ModOptionValueKind : uint8_t {
    Boolean,
    Integer,
    Float,
    String,
    None  ///< value not set / sentinel
};

struct ModOptionValue {
    ModOptionValueKind kind = ModOptionValueKind::None;
    union {
        bool bool_val = false;
        int64_t int_val;
        double float_val;
    };
    chem::string_view str_val;  ///< only valid when kind == String

    ModOptionValue() = default;

    static ModOptionValue make_bool(bool v) {
        ModOptionValue val;
        val.kind = ModOptionValueKind::Boolean;
        val.bool_val = v;
        return val;
    }

    static ModOptionValue make_int(int64_t v) {
        ModOptionValue val;
        val.kind = ModOptionValueKind::Integer;
        val.int_val = v;
        return val;
    }

    static ModOptionValue make_float(double v) {
        ModOptionValue val;
        val.kind = ModOptionValueKind::Float;
        val.float_val = v;
        return val;
    }

    static ModOptionValue make_string(chem::string_view v) {
        ModOptionValue val;
        val.kind = ModOptionValueKind::String;
        val.str_val = v;
        return val;
    }
};

/// A single parsed option from a chemical.mod file, with validation done.
struct ModFileOption {
    chem::string_view key;      ///< e.g. "checks.bounds", "safety"
    ModOptionValue value;       ///< the parsed and validated value
    SourceLocation location;    ///< source location for error reporting
};

// ---------------------------------------------------------------------------
// Option value kind an option descriptor expects
// ---------------------------------------------------------------------------

enum class OptionValueKind : uint8_t {
    Boolean,
    Integer,
    Float,
    String
};

/// Lightweight span of allowed string values (no heap allocation).
struct AllowedStringList {
    std::span<const chem::string_view> values;
};

/// Inclusive integer range.
struct IntegerRange {
    int64_t min;
    int64_t max;
};

/// Inclusive float/double range.
struct FloatRange {
    double min;
    double max;
};

/// Descriptor for a single registered option.
struct OptionDescriptor {
    OptionValueKind value_kind;

    /// Allowed values (interpretation depends on value_kind):
    /// - Boolean: not used (always true/false valid)
    /// - Integer: range.min to range.max inclusive
    /// - Float:   range.min to range.max inclusive
    /// - String:  string_allowed (empty = any string allowed)
    union {
        AllowedStringList string_allowed;
        IntegerRange int_range;
        FloatRange float_range;
    };

    /// Default constructor needed because the anonymous union has a non-trivial
    /// member (AllowedStringList contains std::span). Initialize with IntegerRange
    /// which is trivially constructible.
    OptionDescriptor() : value_kind(OptionValueKind::Boolean), int_range{0, 0} {}

    static OptionDescriptor boolean() {
        OptionDescriptor desc;
        desc.value_kind = OptionValueKind::Boolean;
        return desc;
    }

    static OptionDescriptor integer(int64_t min, int64_t max) {
        OptionDescriptor desc;
        desc.value_kind = OptionValueKind::Integer;
        desc.int_range = { min, max };
        return desc;
    }

    static OptionDescriptor floating(double min, double max) {
        OptionDescriptor desc;
        desc.value_kind = OptionValueKind::Float;
        desc.float_range = { min, max };
        return desc;
    }

    static OptionDescriptor string_val(std::span<const chem::string_view> allowed) {
        OptionDescriptor desc;
        desc.value_kind = OptionValueKind::String;
        desc.string_allowed = { allowed };
        return desc;
    }

    static OptionDescriptor any_string() {
        OptionDescriptor desc;
        desc.value_kind = OptionValueKind::String;
        return desc;
    }
};

// ---------------------------------------------------------------------------
// Option value kind matching / validation helpers
// ---------------------------------------------------------------------------

/// Check if a parsed value kind matches what the descriptor expects.
inline bool value_kind_matches(OptionValueKind desc_kind, ModOptionValueKind parsed_kind) {
    switch (desc_kind) {
        case OptionValueKind::Boolean: return parsed_kind == ModOptionValueKind::Boolean;
        case OptionValueKind::Integer: return parsed_kind == ModOptionValueKind::Integer;
        case OptionValueKind::Float:   return parsed_kind == ModOptionValueKind::Float;
        case OptionValueKind::String:  return parsed_kind == ModOptionValueKind::String;
    }
    return false;
}

/// Validate that the parsed value is within the allowed set of the descriptor.
inline bool validate_allowed(const OptionDescriptor& desc, const ModOptionValue& val) {
    switch (desc.value_kind) {
        case OptionValueKind::Boolean:
            return true; // true and false are always valid

        case OptionValueKind::Integer:
            return val.int_val >= desc.int_range.min && val.int_val <= desc.int_range.max;

        case OptionValueKind::Float:
            return val.float_val >= desc.float_range.min && val.float_val <= desc.float_range.max;

        case OptionValueKind::String: {
            auto& list = desc.string_allowed.values;
            if (list.empty()) return true; // empty = any string allowed
            for (auto& allowed : list) {
                if (allowed == val.str_val) return true;
            }
            return false;
        }
    }
    return false;
}

/// Get a human-readable name for an option value kind.
inline const char* option_value_kind_name(OptionValueKind kind) {
    switch (kind) {
        case OptionValueKind::Boolean: return "boolean";
        case OptionValueKind::Integer: return "integer";
        case OptionValueKind::Float:   return "float";
        case OptionValueKind::String:  return "string";
    }
    return "unknown";
}

// ---------------------------------------------------------------------------
// Option validation helper for parser diagnostics
// ---------------------------------------------------------------------------



// ---------------------------------------------------------------------------
// Global option registry
// ---------------------------------------------------------------------------

/// Returns a reference to the single option registry, built once on first access.
const std::unordered_map<chem::string_view, OptionDescriptor>& get_option_registry();

/// Initialize the registry (called once internally by get_option_registry()).
void init_module_option_registry(
    std::unordered_map<chem::string_view, OptionDescriptor>& registry
);
