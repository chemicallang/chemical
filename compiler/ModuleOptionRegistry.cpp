// Copyright (c) Chemical Language Foundation 2025.

#include "ModuleOptionRegistry.h"

// ---------------------------------------------------------------------------
// Static storage for allowed string views (outlives the registry).
// These are compile-time constants, never freed.
// ---------------------------------------------------------------------------

static constexpr chem::string_view kStackProtectorValues[] = {
    "none", "standard", "strong", "all"
};

static constexpr chem::string_view kHostValues[] = {
    "js", "native"
};

// ---------------------------------------------------------------------------
// Registry initialization
// ---------------------------------------------------------------------------

void init_module_option_registry(
    std::unordered_map<chem::string_view, OptionDescriptor>& registry
) {
    // Integer options (with range restrictions)
    registry["optimization_level"]   = OptionDescriptor::integer(0, 3);

    // String options (with allowed value lists)
    // NOTE: keys use underscores to match struct field names in LabModuleOptions/lab.ch
    registry["stack_protector"]      = OptionDescriptor::string_val(kStackProtectorValues);
    // Spelled as a string in chemical.mod, stored as the integer ABI field
    // `LabModuleOptions::host` / `ModuleOptions.host` (0 = any, 1 = js, 2 = native).
    registry["host"]                 = OptionDescriptor::string_val(kHostValues);
}

// ---------------------------------------------------------------------------
// Singleton accessor (lazy-initialized, thread-safe with static local)
// ---------------------------------------------------------------------------

const std::unordered_map<chem::string_view, OptionDescriptor>& get_option_registry() {
    static std::unordered_map<chem::string_view, OptionDescriptor> registry = []() {
        std::unordered_map<chem::string_view, OptionDescriptor> reg;
        init_module_option_registry(reg);
        return reg;
    }();
    return registry;
}
