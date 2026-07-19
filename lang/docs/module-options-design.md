# Module Options: `chemical.mod` Configuration Syntax

> **Status:** Design Proposal — July 19, 2026
>
> **Problem:** Modules need a way to declare configuration options (e.g., `safe-mode = enforce`,
> `bounds-checks = on`) without requiring a full `build.lab`. The syntax must be
> **fast to parse**, **easy to write**, **scalable**, **translatable to build.lab**,
> and **usable from both the modconv path and the direct-import path** in `LabBuildCompiler`.

---

## Table of Contents

1. [Design Goals & Constraints](#design-goals--constraints)
2. [Proposed Syntax](#proposed-syntax)
3. [Architecture Overview](#architecture-overview)
4. [The Option Registration System](#the-option-registration-system)
5. [Parsing Implementation](#parsing-implementation)
6. [The Struct Mirror: C++ ↔ Chemical](#the-struct-mirror-c--chemical)
7. [ModToLabConverter (modconv) Translation](#modtolabconverter-modconv-translation)
8. [Direct Import (LabBuildCompiler) Handling](#direct-import-labbuildcompiler-handling)
9. [Validation & Error Reporting](#validation--error-reporting)
10. [Performance Considerations](#performance-considerations)
11. [Extensibility & Future Options](#extensibility--future-options)
12. [Comparison of Alternatives Considered](#comparison-of-alternatives-considered)

---

## Design Goals & Constraints

### 1. Fast Parsing

- Must use the **existing `BasicParser`** infrastructure (no new lexer modes, no new token types).
- Must not cause significant overhead for the **many `chemical.mod` files** that exist (currently 34 in `lang/libs/` alone).
- Parsing should be **single-pass, streaming** — no backtracking, no speculative parsing.

### 2. Easy to Write & Read

- Syntax should be **declarative** and **obvious** to a new Chemical user.
- Values should accept **booleans** (`true`/`false`), **integers** (`3`, `42`), and **strings** (`"enforce"`).

### 3. Scalable & Extensible

- Adding a new option requires **no parser changes** — just registration.
- The registration system is a **global map built once at compiler startup** (like the CBI maps).

### 4. Translatable to `build.lab`

- `ModToLabConverter` emits direct **struct field assignment** like `mod.options.checks.bounds = false`.

### 5. Importable Directly (without modconv)

- `LabBuildCompiler::build_module_from_mod_file()` reads parsed options and applies them directly.

### 6. Validated Automatically

- **Unknown option key** → error (compiler knows all valid keys from registration).
- **Wrong value type** → error (e.g., expecting boolean, got string).
- **Value not in allowed set** → error (e.g., "unknown_safety_level" is not in `["off", "warn", "enforce"]`).

### 7. Struct Mirror (C++ ↔ Chemical)

- Options are stored on the C++ `LabModule` struct and the Chemical `Module` interface.
- **Both structs must match** in field order, field names, and field types.
- A prominent comment alerts developers who add/change options to update both sides.

---

## Proposed Syntax

```chmod
// Package declaration (required, as today)
module my_lib

// Source paths (as today)
source "src"

// --- Module Options ---
// Boolean values:
option safety = false;
option checks.bounds = false;

// Integer values:
option optimization-level = 3;

// String values:
option safe-mode = "enforce";
option stack-protector = "strong";

// Imports, links, etc. (as today)
import std
link "m"
```

### Supported Value Types

| Example | Token Type | Parsed As |
|---------|-----------|-----------|
| `option x = true` | `TrueKw` / `FalseKw` | Boolean |
| `option x = false` | `TrueKw` / `FalseKw` | Boolean |
| `option x = 42` | `Number` | Integer |
| `option x = 0` | `Number` | Integer |
| `option x = "hello"` | `String` or `MultilineString` | String |
| `option checks.bounds = false` | Dot-separated key + boolean | Boolean (key path) |

### Why the `option` keyword?

- **Clear intent**: grep-able, searchable, unambiguous.
- **No ambiguity with `.ch` file variables** (where `var` / `const` mean something else).
- **Consistent with SQL/CSS-style declarative config** — readable at a glance.

### Why dotted keys (`checks.bounds`)?

- Maps directly to **nested struct field access** in build.lab translation.
- Enables **logical grouping** of related options without creating a flat namespace.
- The dot path defines the struct nesting: `checks` is a nested struct, `bounds` is its field.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                 Global Option Registry                       │
│  (built once at startup, like CBI maps)                     │
│                                                             │
│  map<key, OptionDescriptor>                                 │
│                                                             │
│  "safety"        → { kind: Boolean }                        │
│  "checks.bounds" → { kind: Boolean }                        │
│  "safe-mode"     → { kind: String, allowed: ["off","warn",  │
│                                 "enforce"] }                 │
│  "opt-level"     → { kind: Integer, range: [0, 3] }        │
└───────────────────────┬─────────────────────────────────────┘
                        │ lookup + validate
                        ▼
┌──────────────────────────────────────────────────────────────┐
│                  chemical.mod Parser                         │
│  (BasicParser::parseOptionStmt)                              │
│                                                              │
│  Parses: option <key> = <value>                              │
│  Validates: key exists, value kind matches, value is allowed │
│  Stores: ModFileOption { key_path, parsed_value }            │
└─────────┬────────────────────┬──────────────────────────────┘
          │                    │
          ▼                    ▼
┌──────────────────┐  ┌──────────────────────────────┐
│ ModToLabConverter │  │ build_module_from_mod_file() │
│ (modconv path)   │  │ (direct import path)         │
│                  │  │                              │
│ Emits:           │  │ Sets:                        │
│ mod.options.     │  │ module->options fields       │
│   checks.bounds  │  │ directly from parsed values  │
│   = false        │  │                              │
└──────────────────┘  └──────────────────────────────┘
```

---

## The Option Registration System

### Overview

A single global registry (`std::unordered_map<chem::string_view, OptionDescriptor>` or
a simple sorted array for cache-friendly lookup) is built once when the compiler starts.

**Why global?**
- Built once, shared across all `.mod` parses — zero per-file registration overhead.
- Same pattern used for CBI maps (`prepare_cbi_maps` in `CBI.cpp`).
- Easy to audit: all valid options are in one place.

### Data Structures

```cpp
// compiler/ModuleOptionRegistry.h

/// The kind of value an option accepts.
enum class OptionValueKind : uint8_t {
    Boolean,
    Integer,
    Float,
    String
};

/// AllowedStringList: a lightweight span of static string views.
/// No std::string allocations — points to static data.
struct AllowedStringList {
    std::span<const chem::string_view> values;
};

/// IntegerRange: inclusive min/max for integer options.
struct IntegerRange {
    int64_t min;
    int64_t max;
};

/// The descriptor for a single registered option.
struct OptionDescriptor {
    OptionValueKind value_kind;
    
    /// Allowed values (interpretation depends on value_kind):
    /// - Boolean: not used (always true/false)
    /// - Integer: range.min, range.max (inclusive), or empty span for no restriction
    /// - String: list of allowed strings
    union {
        AllowedStringList string_allowed;
        IntegerRange int_range;
    };
};

/// The parsed value of an option (stored as parsed from .mod file).
enum class ModOptionValueKind : uint8_t {
    Boolean,
    Integer,
    String
};

struct ModOptionValue {
    ModOptionValueKind kind;
    union {
        bool bool_val;
        int64_t int_val;
        chem::string_view str_val;
    };
};
```

### Registration

```cpp
// compiler/ModuleOptionRegistry.cpp

// Static storage for allowed string views (must outlive the registry)
static constexpr chem::string_view kSafeModeValues[] = {
    "off", "warn", "enforce"
};
static constexpr chem::string_view kStackProtectorValues[] = {
    "none", "standard", "strong", "all"
};

void init_module_option_registry(
    std::unordered_map<chem::string_view, OptionDescriptor>& registry
) {
    // Boolean options
    registry["safety"] = { .value_kind = OptionValueKind::Boolean };
    registry["checks.bounds"] = { .value_kind = OptionValueKind::Boolean };
    registry["checks.overflow"] = { .value_kind = OptionValueKind::Boolean };
    registry["checks.null"] = { .value_kind = OptionValueKind::Boolean };
    
    // Integer options (with range)
    registry["optimization-level"] = {
        .value_kind = OptionValueKind::Integer,
        .int_range = { .min = 0, .max = 3 }
    };
    
    // String options (with allowed values)
    registry["safe-mode"] = {
        .value_kind = OptionValueKind::String,
        .string_allowed = { kSafeModeValues }
    };
    registry["stack-protector"] = {
        .value_kind = OptionValueKind::String,
        .string_allowed = { kStackProtectorValues }
    };
}
```

**Where is `init_module_option_registry()` called?**
- In `CompilerMain.cpp` or `LabBuildCompiler` constructor — once at startup.
- Or lazily on first `.mod` parse (with a `std::once_flag`).

---

## Parsing Implementation

### Changes to `parser/Parser.cpp` — `BasicParser::parseModuleFile()`

```cpp
// In parseModuleFile(), add to the Identifier hash-switch:
case hash_fn("option"):
    parseOptionStmt(allocator, data);
    break;
```

### New method: `BasicParser::parseOptionStmt()`

```cpp
bool BasicParser::parseOptionStmt(ASTAllocator& allocator, ModuleFileData& data) {
    // "option" has already been consumed by the caller
    
    // ---- Parse the key path ----
    // The key can be dotted: "checks.bounds" = parse access chain
    // We parse identifiers separated by dots.
    
    auto first_id = consumeIdentifierOrKeyword();
    if (!first_id) {
        error() << "expected an option key after 'option'";
        return false;
    }
    
    // Build the key path: join identifiers with dots
    chem::string key_view = allocate_view(allocator, first_id->value);
    while (consumeToken(TokenType::DotSym)) {
        auto next_id = consumeIdentifierOrKeyword();
        if (!next_id) {
            error() << "expected identifier after '.' in option key";
            return false;
        }
        // Re-allocate the key with dot appended
        // (We can't easily append to a string_view, so we build a string)
        // Actually, we cheat: just store the path parts for later reconstruction
        // For now, we limit to max 2 levels (flat and one-level nested)
        ...
    }
    
    if (!consumeToken(TokenType::EqualSym)) {
        error() << "expected '=' after option key '" << key << "'";
        return false;
    }
    
    // ---- Parse the value ----
    // We parse the value as a typed AST value (BoolValue, IntNumValue, StringValue)
    // using the existing token-level parsing.
    
    ModOptionValue parsed_value;
    SourceLocation value_location;
    
    switch (token->type) {
        case TokenType::TrueKw: {
            parsed_value = ModOptionValue { .kind = ModOptionValueKind::Boolean, .bool_val = true };
            value_location = loc_single(token);
            token++;
            break;
        }
        case TokenType::FalseKw: {
            parsed_value = ModOptionValue { .kind = ModOptionValueKind::Boolean, .bool_val = false };
            value_location = loc_single(token);
            token++;
            break;
        }
        case TokenType::Number: {
            // Parse as arbitrary-precision integer (store as int64)
            // In BasicParser, we can use a simple strtoll
            char* end = nullptr;
            int64_t val = strtoll(token->value.data(), &end, 0);
            if (end == token->value.data()) {
                error() << "invalid integer value for option";
                return false;
            }
            parsed_value = ModOptionValue { .kind = ModOptionValueKind::Integer, .int_val = val };
            value_location = loc_single(token);
            token++;
            break;
        }
        case TokenType::String:
        case TokenType::MultilineString: {
            auto str = parseString(allocator);
            if (!str.has_value()) {
                error() << "expected a string value for option";
                return false;
            }
            parsed_value = ModOptionValue { .kind = ModOptionValueKind::String, .str_val = str.value() };
            value_location = loc_single(token);
            break;
        }
        default:
            error() << "unexpected value type for option (expected boolean, integer, or string)";
            return false;
    }
    
    // ---- Validate against the option registry ----
    const auto& registry = get_option_registry(); // singleton, built once at startup
    auto it = registry.find(key);
    if (it == registry.end()) {
        error() << "unknown option '" << key << "'";
        return false;
    }
    
    const auto& desc = it->second;
    
    // Check value kind matches
    if (!value_kind_matches(desc.value_kind, parsed_value.kind)) {
        error() << "option '" << key << "' expects a " << kind_name(desc.value_kind)
                << " value, got " << kind_name(parsed_value.kind);
        return false;
    }
    
    // Check allowed values
    if (!validate_allowed(desc, parsed_value)) {
        error() << "value '" << value_to_string(parsed_value) << "' is not valid for option '"
                << key << "'";
        return false;
    }
    
    // ---- Store the validated option ----
    data.options.emplace_back(ModFileOption {
        .key = key,
        .value = parsed_value,
        .location = value_location
    });
    
    return true;
}
```

### What about `var` / `const` keywords in `.mod`?

The existing parser currently has a dead case:

```cpp
case TokenType::VarKw:
case TokenType::ConstKw:
    // TODO handle these
```

These remain as-is. The `option` keyword is the intended syntax for module configuration.
`var` and `const` remain unsupported (and will error if encountered by the default case).

---

## The Struct Mirror: C++ ↔ Chemical

### The Core Design Decision

The user's options are translated to **direct struct field assignment** in the build.lab:

```chemical
// In generated build.lab (from chemical.mod):
mod.options.checks.bounds = false;
mod.options.safe_mode = std::string_view("enforce");
```

This means there must be a **Chemical struct** that `mod.options` points to, and a
**matching C++ struct** on `LabModule` that the direct import path fills in.

### The Chemical Struct (in lab module)

```chemical
// lang/libs/lab/src/lab.ch

/// Module-level options struct.
///
/// ╔══════════════════════════════════════════════════════════════╗
/// ║   WARNING: This struct is mirrored in C++                   ║
/// ║   (compiler/lab/LabModuleOptions.h)                        ║
/// ║                                                             ║
/// ║   - Field ORDER must match                                 ║
/// ║   - Field NAMES must match                                 ║
/// ║   - Field TYPES must match                                 ║
/// ║                                                             ║
/// ║   If you add, remove, or change a field, update BOTH        ║
/// ║   the C++ struct AND this struct.                          ║
/// ╚══════════════════════════════════════════════════════════════╝
pub struct ModuleChecksOptions {
    var bounds : bool = false;
    var overflow : bool = false;
    var null : bool = false;
}

/// ╔══════════════════════════════════════════════════════════════╗
/// ║   WARNING: Mirrored in C++ (LabModuleOptions.h)            ║
/// ╚══════════════════════════════════════════════════════════════╝
pub struct ModuleOptions {
    var safety : bool = true;
    var checks : ModuleChecksOptions;
    var safe_mode : std::string_view = std::string_view();
    var stack_protector : std::string_view = std::string_view();
    var optimization_level : int = 0;
}
```

And the `Module` interface (which is compiler-side) gets an `options` accessor:

```chemical
@compiler.interface
public interface Module {
    // ... existing methods ...
    
    func getOptions(&self) : *mut ModuleOptions
    func getOptionsPtr(&self) : *mut ModuleOptions
}
```

### The C++ Struct (in compiler)

```cpp
// compiler/lab/LabModuleOptions.h

/// ╔══════════════════════════════════════════════════════════════╗
/// ║   WARNING: This struct is mirrored in Chemical              ║
/// ║   (lang/libs/lab/src/lab.ch)                               ║
/// ║                                                             ║
/// ║   - Field ORDER must match                                 ║
/// ║   - Field NAMES must match                                 ║
/// ║   - Field TYPES must match                                 ║
/// ║                                                             ║
/// ║   If you add, remove, or change a field, update BOTH        ║
/// ║   the Chemical struct AND this struct.                     ║
/// ╚══════════════════════════════════════════════════════════════╝

struct LabModuleChecksOptions {
    bool bounds = false;
    bool overflow = false;
    bool null = false;
};

struct LabModuleOptions {
    bool safety = true;
    LabModuleChecksOptions checks;
    chem::string_view safe_mode;       // empty = not set
    chem::string_view stack_protector; // empty = not set
    int64_t optimization_level = 0;
};
```

### Adding `options` to `LabModule`

```cpp
// compiler/lab/LabModule.h

struct LabModule {
    // ... existing fields ...
    
    /// Module options (set from chemical.mod or build.lab).
    /// WARNING: The C++ struct LabModuleOptions mirrors the Chemical
    /// struct ModuleOptions in lab/src/lab.ch. Keep them in sync.
    LabModuleOptions options;
};
```

### Adding `getOptions` to the CBI bridge

In `compiler/cbi/bindings/LabCBIAddons.h`:

```cpp
extern "C" {
    // ... existing ...
    
    LabModuleOptions* ModulegetOptions(LabModule* self);
}
```

In `compiler/cbi/bindings/LabCBIAddons.cpp`:

```cpp
LabModuleOptions* ModulegetOptions(LabModule* self) {
    return &self->options;
}
```

Registration in `CBI.cpp`:

```cpp
{ "lab_ModulegetOptions", (void*) ModulegetOptions },
```

And in the Chemical `lab.ch` interface:

```chemical
@compiler.interface
public interface Module {
    // ... existing methods ...
    func getOptions(&self) : *mut ModuleOptions
}
```

---

## ModToLabConverter (modconv) Translation

### How `option` statements get translated to build.lab

For each parsed option in `ModuleFileData.options`, the `convertToBuildLab()` function
emits a struct field assignment.

```cpp
// In compiler/lab/mod_conv/ModToLabConverter.cpp

void convertToBuildLab(const ModuleFileData& data, std::ostream& output) {
    // ... existing code: create module, set cached ...

    // --- Module Options ---
    // Convert each option to a struct field assignment on mod.options
    for (auto& opt : data.options) {
        // The key "checks.bounds" becomes "mod.options.checks.bounds"
        output << "\tmod.getOptions().";
        
        // Emit the dotted path as nested struct access
        // (The key was validated during parsing, so we know it's valid)
        output << opt.key << " = ";
        
        // Emit the value
        switch (opt.value.kind) {
            case ModOptionValueKind::Boolean:
                output << (opt.value.bool_val ? "true" : "false");
                break;
            case ModOptionValueKind::Integer:
                output << opt.value.int_val;
                break;
            case ModOptionValueKind::String:
                output << "std::string_view(\"" << opt.value.str_val << "\")";
                break;
        }
        
        output << ";\n";
    }

    // ... rest of existing code ...
    output << "\treturn mod;\n";
    output << "}\n\n";
}
```

### Example translation

**Input (`chemical.mod`):**
```chmod
module my_app
source "src"
option safety = false
option checks.bounds = true
option safe-mode = "enforce"
option optimization-level = 2
import std
```

**Output (generated `build.lab`):**
```chemical
import lab;
import std;
import "@std/build.lab" as __mod_0_stmt;

public func build(ctx : *mut BuildContext, __chx_job : *mut LabJob) : *mut Module {
    var __curr_lab_path = lab::get_my_path();
    const __chx_already_exists = ctx.get_cached(__chx_job, &__curr_lab_path);
    if(__chx_already_exists != null) { return __chx_already_exists; }
    
    const deps : []ModuleDependency = [
        ModuleDependency { module: __mod_0_stmt.build(ctx, __chx_job), info: null },
    ];
    const mod = ctx.new_package(ModuleType.Directory, PackageKind.Application, "", "my_app", std::span<ModuleDependency>(deps, 1));
    ctx.set_cached(__chx_job, &__curr_lab_path, mod)
    
    // --- Module Options ---
    mod.getOptions().safety = false;
    mod.getOptions().checks.bounds = true;
    mod.getOptions().safe-mode = std::string_view("enforce");
    mod.getOptions().optimization-level = 2;
    
    ctx.add_path(mod, lab::rel_path_to("src").to_view());
    return mod;
}
```

---

## Direct Import (LabBuildCompiler) Handling

### In `build_module_from_mod_file()`

When a `.mod` file is imported directly (without modconv), the parsed options from
`ModuleFileData.options` are applied directly to the `LabModule` struct.

```cpp
// In compiler/lab/LabBuildCompiler.cpp

LabModule* LabBuildCompiler::build_module_from_mod_file(
        LabBuildContext& context,
        const std::string_view& modFilePathView,
        LabJob* job
) {
    // ... existing code: parse mod file into modFileData ...

    // --- NEW: Apply module options directly to LabModule ---
    if (!modFileData.options.empty()) {
        apply_module_options_to_struct(&modPtr->options, modFileData.options);
    }

    // ... existing code: return modPtr ...
}
```

### `apply_module_options_to_struct()`

```cpp
// In compiler/lab/LabBuildCompiler.cpp or a new file

void apply_module_options_to_struct(
    LabModuleOptions* opts,
    const std::vector<ModFileOption>& options
) {
    for (auto& opt : options) {
        // Dispatch based on known key paths
        // (These must match the field paths defined in the registration)
        if (opt.key == "safety") {
            CHEM_ASSERT(opt.value.kind == ModOptionValueKind::Boolean);
            opts->safety = opt.value.bool_val;
        } else if (opt.key == "checks.bounds") {
            opts->checks.bounds = opt.value.bool_val;
        } else if (opt.key == "checks.overflow") {
            opts->checks.overflow = opt.value.bool_val;
        } else if (opt.key == "checks.null") {
            opts->checks.null = opt.value.bool_val;
        } else if (opt.key == "safe-mode") {
            opts->safe_mode = opt.value.str_val;
        } else if (opt.key == "stack-protector") {
            opts->stack_protector = opt.value.str_val;
        } else if (opt.key == "optimization-level") {
            opts->optimization_level = opt.value.int_val;
        }
        // No "else" needed — the parser already validated the key exists
    }
}
```

---

## Validation & Error Reporting

### Parser-Level Validation

| Condition | Behavior |
|-----------|----------|
| `option;` (no key) | Error: "expected an option key after 'option'" |
| `option .foo = ...` (starts with dot) | Error: expected identifier |
| `option foo. = ...` (ends with dot) | Error: expected identifier after '.' |
| `option unknown-opt = true` (not registered) | Error: "unknown option 'unknown-opt'" |
| `option safety = 42` (wrong type) | Error: "option 'safety' expects a boolean value, got integer" |
| `option safe-mode = "unknown"` (not in allowed list) | Error: "value 'unknown' is not valid for option 'safe-mode'" |
| `option optimization-level = 5` (out of range) | Error: "value 5 is not valid for option 'optimization-level'" |

### Build-Level Warnings (for the modconv path)

If someone hand-writes a build.lab that sets an invalid option, the setter functions
should warn:

```
[lab] warning: unknown module option 'unknown-key' set on module 'my_mod'
```

---

## Performance Considerations

### Parsing

- Zero cost for modules without `option` statements — the hash switch falls through.
- Each `option` is ~3-5 tokens parsed. Overhead is negligible for the typical 0-10 options.
- Validation uses the global registry (unordered_map lookup, O(1) average).

### Registration Map

- Built once at startup (or on first use).
- Uses `chem::string_view` keys pointing to static data — no dynamic allocation after init.
- Can be sorted at init time for binary-search lookup (more cache-friendly than hash map).

### Memory

- `ModFileOption` stores key (`string_view` aliased to arena), value (16 bytes), and location (8 bytes).
- ~40 bytes per option, allocated on the module file arena — freed when the parse is done.
- `LabModuleOptions` is a small struct (~48 bytes) embedded directly in `LabModule`.

### Runtime

- Options are **parsed once**, **validated once**, then **applied once**.
- No option lookups during compilation of module files — the struct fields are accessed directly.

---

## Extensibility & Future Options

### Adding a New Option

1. **Register** it in `init_module_option_registry()`.
2. **Add field** to `LabModuleOptions` (C++) and `ModuleOptions` (Chemical struct — keep in sync).
3. **Add handler** in `apply_module_options_to_struct()`.
4. **No parser changes** needed.

### Future Option Candidates

| Key | Kind | Allowed Values | Description |
|-----|------|---------------|-------------|
| `safety` | Boolean | `true`/`false` | Module-level safety enforcement on/off |
| `checks.bounds` | Boolean | `true`/`false` | Insert array bounds checks |
| `checks.overflow` | Boolean | `true`/`false` | Insert overflow checks |
| `checks.null` | Boolean | `true`/`false` | Insert null checks |
| `safe-mode` | String | `"off"`, `"warn"`, `"enforce"` | Safety mode granularity |
| `stack-protector` | String | `"none"`, `"standard"`, `"strong"`, `"all"` | Stack canary protection |
| `optimization-level` | Integer | `0`-`3` | Per-module optimization hint |
| `optimize` | String | `"size"`, `"speed"`, `"none"` | Optimization goal |
| `visibility` | String | `"default"`, `"hidden"` | Default symbol visibility |
| `no-std` | Boolean | `true`/`false` | Don't link std implicitly |
| `panic` | String | `"abort"`, `"unwind"` | Panic strategy |

---

## Comparison of Alternatives Considered

### Alternative A: Functional Approach (rejected)

Instead of struct field assignment, use setter functions:

```chemical
mod.options.setSafeMode("enforce");
mod.options.setChecksBounds(true);
```

**Pros:** ABI stable — field order/type changes don't break binary compatibility.
**Cons:**
- One function per option = explosion of CBI symbols.
- `tcc_add_symbol()` per function incurs overhead.
- More verbose generated code.
- Each new option requires a new C function + CBI registration + interface method.

### Alternative B: Flat key-value map (rejected)

```cpp
std::unordered_map<chem::string, chem::string> option_strings;
```

**Pros:** Zero structural coupling between C++ and Chemical.
**Cons:**
- Values stored as strings → consumers must parse them again.
- No type safety — a boolean option might accidentally get `"yes"` instead of `"true"`.
- Translation requires string conversion at every use site.

### Alternative C: Annotation-based (rejected)

```chmod
@option(safety = false)
```

**Pros:** Looks familiar.
**Cons:**
- Annotations in Chemical are for AST nodes, not module config.
- Requires annotation argument parsing in the .mod context.
- Harder to grep for than a dedicated keyword.

### Chosen: Struct Mirror (this design)

**Pros:**
- Type-safe at the compiler level.
- Direct field access is fast — no function call overhead.
- The build.lab translation is simple string concatenation.
- The big warning comment makes the coupling explicit.
- Any struct change that breaks the Chemical side will cause a build error in `lab.ch`,
  alerting developers immediately.

**Cons:**
- Binary coupling between C++ and Chemical struct layout.
- Mitigated by: (a) the prominent warning comment, (b) the fact that both are in the
  same repository, (c) struct changes are rare once options stabilize.
