// Copyright (c) Chemical Language Foundation 2025.

#include "ModuleOptionRegistry.h"

// ---------------------------------------------------------------------------
// Static storage for allowed string views (outlives the registry).
// These are compile-time constants, never freed.
// ---------------------------------------------------------------------------

static constexpr chem::string_view kSafeModeValues[] = {
    "off", "warn", "enforce"
};

static constexpr chem::string_view kStackProtectorValues[] = {
    "none", "standard", "strong", "all"
};

// ---------------------------------------------------------------------------
// Registry initialization
// ---------------------------------------------------------------------------

void init_module_option_registry(
    std::unordered_map<chem::string_view, OptionDescriptor>& registry
) {
    // Boolean options (simple on/off switches)
    registry["safety"]               = OptionDescriptor::boolean();
    registry["checks.bounds"]        = OptionDescriptor::boolean();
    registry["checks.overflow"]      = OptionDescriptor::boolean();
    registry["checks.null"]          = OptionDescriptor::boolean();

    // Integer options (with range restrictions)
    registry["optimization_level"]   = OptionDescriptor::integer(0, 3);

    // String options (with allowed value lists)
    // NOTE: keys use underscores to match struct field names in LabModuleOptions/lab.ch
    registry["safe_mode"]            = OptionDescriptor::string_val(kSafeModeValues);
    registry["stack_protector"]      = OptionDescriptor::string_val(kStackProtectorValues);
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
