---
name: Diagnostics and Error Reporting
description: Comprehensive guide to the Chemical compiler's diagnostic system — how errors, warnings, and informational messages are collected, reported, and formatted.
---

# Diagnostics and Error Reporting

The Chemical compiler has a sophisticated diagnostics system that captures errors, warnings, and informational messages during all compilation phases. Diagnostics are collected as `Diag` values by `Diagnoser` (and the location-aware `ASTDiagnoser`) objects.

## Architecture

### Key Files

| File | Purpose |
|------|---------|
| `core/diag/Diagnoser.h` | `Diagnoser` base class — `error_count`, `diagnostics`, `has_errors()`, `print_diagnostics()` |
| `core/diag/Diagnostic.h` | `Diag` struct, `DiagTag`, `DiagRelatedInfo`, `TextReplacement` |
| `core/diag/DiagSeverity.h` | `DiagSeverity` enum: `Error`, `Warning`, `Information`, `Hint` |
| `core/source/SourceLocation.h` | `SourceLocation` class wrapping an encoded `uint64_t` |
| `core/source/LocationManager.h/.cpp` | Encodes/decodes source locations, owns file paths |
| `compiler/ASTDiagnoser.h` | `ASTDiagnoser` — location-aware diagnoser extending `Diagnoser` |
| `compiler/ASTDiagnoser.cpp` | Location decoding + special diagnostic helpers |
| `compiler/ASTDiag.h` | Small `ASTDiag { std::string message; DiagSeverity severity; }` struct |
| `compiler/CodegenOptions.h` | Codegen options that affect error behavior |
| `compiler/SanitizerOptions.h` | Sanitizer-related error handling |
| `ast/base/ASTNode.h` | Base AST node — provides `encoded_location()` |
| `compiler/cbi/bindings/ASTDiagnoserCBI.h/.cpp` | CBI binding (`ASTDiagnosererror`) exposed to compiler plugins |

## ASTDiagnoser

The `ASTDiagnoser` class is the primary location-aware collector. It extends the plain `Diagnoser` (which owns the diagnostics and error count) and adds `SourceLocation` decoding via a `LocationManager`:

```cpp
class ASTDiagnoser : public Diagnoser {
public:
    LocationManager& loc_man;                    // decodes SourceLocation → path + Position range

    ASTDiagnoser(LocationManager& loc_man);

    void location_diagnostic(const chem::string_view& message, SourceLocation loc, DiagSeverity severity);
    Diag& empty_diagnostic(SourceLocation loc, DiagSeverity severity);  // empty Diag to append to

    // Create an empty Diag (severity set) to append to with operator<<:
    Diag& error(SourceLocation loc);     // DiagSeverity::Error
    Diag& warning(SourceLocation loc);   // DiagSeverity::Warning
    Diag& warn(SourceLocation loc);      // alias of warning()
    Diag& info(SourceLocation loc);      // DiagSeverity::Information
    Diag& hint(SourceLocation loc);      // DiagSeverity::Hint

    // Message overloads: error(msg, loc), warn(msg, loc), info(msg, loc);
    // and templated node overloads that use node->encoded_location().

    void dup_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node);
    void unsatisfied_type_error(Value* value, BaseType* type);

    // Inherited from Diagnoser:
    unsigned int error_count;            // incremented for Error severity
    std::vector<Diag> diagnostics;
    bool has_errors();                   // error_count > 0
    void reset_diagnostics();            // exposed on ASTDiagnoser as reset_errors()
};
```

> There is **no** `note()` method, no `max_severity` field, no `has_warnings()`, and no
> `error_count()` method — `error_count` is a public field on `Diagnoser`. Diagnostics with
> `Hint` severity are used for debug locations.

### Diagnostic Structure

`Diag` lives in `core/diag/Diagnostic.h` (not in `ASTDiag.h`) and carries a `Range` plus optional metadata:

```cpp
class Diag {
public:
    Range range;                                  // start/end Position (line + character)

    std::optional<DiagSeverity> severity;         // omitted = client-interpreted
    std::optional<std::string> path_url;          // file path (or documentation URL)
    std::string message;                          // primary text

    std::vector<TextReplacement> fixits_;         // non-serialized fix-its
    std::optional<std::vector<DiagTag>> tags;     // Unnecessary / Deprecated
    std::optional<std::vector<DiagRelatedInfo>> relatedInformation;

    // Append to `message`:
    Diag& operator<<(const char*);
    Diag& operator<<(const chem::string_view&);
    Diag& operator<<(const std::string_view&);
    Diag& operator<<(std::string&);
    Diag& operator<<(char);

    void format(std::ostream& os, const chem::string_view& path, const chem::string_view& tag) const;
    std::ostream& ansi(std::ostream& os, const chem::string_view& path,
                       const chem::string_view& tag = "Diagnostic") const;
};
```

### Severity Levels

```
Error       → increments Diagnoser::error_count; compilation should stop
Warning     → printed, does not fail compilation
Information → informational message
Hint        → additional context (also used by print_debug_location)
```

The enum is `DiagSeverity`, so the spellings are `DiagSeverity::Error`,
`Warning`, `Information` (not `Info`), and `Hint` (not `Note`).

## Source Location System

### SourceLocation

`SourceLocation` (`core/source/SourceLocation.h`) is a thin wrapper around a 64-bit encoded value:

```cpp
class SourceLocation {
public:
    uint64_t encoded;

    bool isValid() const;    // encoded != 0
    bool isInvalid() const;  // encoded == 0
};

inline constexpr uint64_t ZERO_LOC = 0;   // fileId 0 is reserved as "unknown / invalid"
```

### Encoded Location Layout

`LocationManager::addLocation(fileId, lineStart, charStart, lineEnd, charEnd)` packs the
range into 63 bits; bit 63 is the overflow indicator (when set, the low 63 bits index
`LocationManager::locations`). The bit widths are declared on `LocationManager`
(`FILE_ID_BITS`, `LINE_START_BITS`, `CHAR_START_BITS`, `LINE_END_OFFSET_BITS`, `CHAR_END_BITS`):

```
Bits 53-62: file id            (10 bits)
Bits 35-52: line start         (18 bits)
Bits 23-34: char start         (12 bits)
Bits 12-22: line-end offset    (11 bits, stored as lineEnd - lineStart)
Bits  0-11: char end           (12 bits)
Bit  63:    overflow indicator — when set, the low 63 bits index LocationManager::locations
```

Decode with `getLocation(SourceLocation)` (returns `LocationData { fileId, lineStart, charStart,
lineEnd, charEnd }`) or `getLocationPos(SourceLocation)` (returns `{ fileId, Position start,
Position end }`). `getLineStartFast()` skips the full decode when only the line is needed.

### LocationManager

```cpp
class LocationManager {
    std::vector<LocationData> locations;              // overflow table for ranges too large to pack
    tsl::ordered_map<std::string, bool> file_paths;   // file id → path (insertion order)

    unsigned int encodeFile(const std::string& filePath);
    int encodeExistingFile(const std::string& filePath);
    std::string_view getPathForFileId(unsigned int fileId);

    uint64_t addLocation(uint32_t fileId, uint32_t lineStart, uint32_t charStart,
                         uint32_t lineEnd, uint32_t charEnd);
    LocationData getLocation(uint64_t data) const;
    LocationData getLocation(SourceLocation loc) const;
    LocationPosData getLocationPos(SourceLocation loc) const;

    std::string formatLocation(SourceLocation location);
};
```

### Encoded Location API

```cpp
// Encode (note: two characters, unlike a classic line/column pair):
uint64_t loc = loc_man.addLocation(fileId, lineStart, charStart, lineEnd, charEnd);

// Decode:
SourceLocation sloc(loc);
auto data = loc_man.getLocation(sloc);              // LocationData
auto pos  = loc_man.getLocationPos(sloc);           // LocationPosData
```

## Diagnostic Collection Flow

### Per-Phase Collection

Each phase returns its diagnostics as a `std::vector<Diag>` inside a result struct; there is
**no** `ASTDiagnoser::merge`. The phase-local visitor/diagnoser owns the diagnostics and moves
them out when the phase finishes:

```cpp
// compiler/symres/LinkSignatureAPI.h
struct SymResSignatureResult {
    std::vector<std::pair<TypealiasStatement*, std::vector<TypeLoc>>> inline_instantiations;
    bool has_errors;
    std::vector<Diag> diagnostics;
};

// compiler/symres/SymResLinkBodyAPI.h
struct SymResLinkBodyResult {
    bool has_errors;
    std::vector<Diag> diagnostics;
};

// compiler/ASTProcessor.cpp — type verification per file
struct TypeVerifyFileResult {
    bool has_errors = false;
    std::vector<Diag> diagnostics;
};
```

### Reporting Flow

Results are printed (and discarded) through the static `Diagnoser::print_diagnostics`, passing
a phase tag:

```cpp
Diagnoser::print_diagnostics(result.diagnostics, chem::string_view(abs_path), "SymRes:link_sig");
```

Serial phases can use `SymbolResolver` directly, since it extends `ASTDiagnoser` (and therefore
`Diagnoser`): `resolver->has_errors()` / `resolver->reset_errors()`.

### Parallel Safety

Parallel file tasks use a local diagnoser per task, move its `diagnostics` into the task result,
and print under a mutex. For example, `type_verify_file_task` constructs its own
`ASTDiagnoser(processor->loc_man)`, and `ASTProcessor` locks `print_mutex` before calling
`Diagnoser::print_diagnostics`.

## Error Messages

### Format

`Diag::ansi()` renders a single-line, ANSI-coloured diagnostic (line/column are 1-based):

```
[<tag>] error: <message> at <path>:<line>:<column>
```

Warnings render `[<tag>] warn: ...`, info `[<tag>] info: ...`, and hints
`[<tag>] hint: ...`. `Diag::format()` is the non-coloured variant
(`[<tag>] <message> at <path>:<range>`). There is no multi-line snippet/caret rendering and no
`= note:` continuation lines — related context is carried in `Diag::relatedInformation`.

### Common Error Types

| Error Pattern | Source | Example |
|---------------|--------|---------|
| Symbol not found | `SymbolResolver::find()` | `error: cannot find symbol 'foo' in current scope` |
| Duplicate symbol | `SymbolResolver::declare()` | `error: duplicate symbol 'bar'` |
| Type mismatch | `TypeVerify` | `error: expected 'int', got 'float'` |
| Cannot move | `SymResLinkBody` | `error: cannot move value of type 'int'` |
| Access violation | `SymResLinkBody` | `error: 'x' is private and cannot be accessed` |
| Unsafe operation in safe context | `SymResLinkBody` | `error: unsafe operation not allowed outside unsafe block` |
| Generic instantiation failed | `GenericInstantiator` | `error: failed to instantiate generic 'Foo<int>'` |
| Invalid comptime expression | `Interpreter` | `error: cannot evaluate expression at compile time` |

## Plugin Diagnostics

Compiler plugins report diagnostics through the CBI binding exposed by
`compiler/cbi/bindings/ASTDiagnoserCBI.h/.cpp`. The binding is a single no-mangle symbol:

```cpp
extern "C" {
    void ASTDiagnosererror(ASTDiagnoser* diagnoser, chem::string_view* msg, uint64_t loc);
}

void ASTDiagnosererror(ASTDiagnoser* diagnoser, chem::string_view* msg, uint64_t loc) {
    diagnoser->error(*msg, loc);
}
```

The Chemical-side interface is `ASTDiagnoser` in `lang/libs/compiler/src/ASTDiagnoser.ch`
(`func error(&self, msg : &std::string_view, loc : ubigint)`), and CBI exposes it to plugins as
`compiler_ASTDiagnosererror`. There is no `cbi_report_warning/info/note` family.

## ASTDiag.h

`ASTDiag.h` only declares a small value type — there are no predefined message helpers here:

```cpp
// compiler/ASTDiag.h
struct ASTDiag {
    std::string message;
    DiagSeverity severity;
};
```

All diagnostics are created through `ASTDiagnoser` (`error(...)`, `warn(...)`, `info(...)`,
`hint(...)`) or the `Diagnoser` base API.

## Print/Buffer Strategy

### Current Approach

Diagnostics are collected during a phase and printed after it, via the static
`Diagnoser::print_diagnostics`:

```cpp
// After a phase (optionally under print_mutex in parallel phases):
Diagnoser::print_diagnostics(result.diagnostics, chem::string_view(abs_path), "SymRes:link_sig");

// Or print a diagnoser's own collected diagnostics:
diagnoser.print_diagnostics(path, "SymRes");
```

`print_diagnostics` sends `Error` severities to `stderr` and everything else to `stdout`.

### Future Improvements

1. **Batch and sort** diagnostics by file/line/column
2. **Deduplicate** repeated errors
3. **Colored output** based on severity
4. **JSON output** for IDE integration
5. **Compressed output** — omit hints unless verbose (-v)
6. **Diagnostic groups** — group related errors together
7. **Error recovery** — continue compilation to find more errors
8. **Suppression** — allow suppressing specific warnings

## Debugging Diagnostics

### Adding a Diagnostic

```cpp
// In any AST visitor / Value / ASTNode:
diagnoser.error(node->encoded_location()) << "this feature is not yet implemented";
// Or with a message and a location:
diagnoser.warn(node->encoded_location(), "this operation is deprecated, use 'new_thing' instead");
// Or a message and any node exposing encoded_location():
diagnoser.error("expected an integer", someValue);
```

### Checking for Errors

```cpp
if(diagnoser.has_errors()) {
    // Skip codegen, report errors
    return;
}
```

### Counting Errors

```cpp
unsigned int err_count = diagnoser.error_count;   // public field, not a method
```

## Performance Considerations

1. **Owned messages**: `Diag::message` is a `std::string`; file paths are deduplicated in `LocationManager::file_paths`
2. **Move semantics**: Diagnostics are moved (not copied) out of phase-local diagnosers
3. **Per-file abort**: `ASTProcessorOptions::stop_on_file_error` decides whether a module aborts after a phase reports errors
4. **Per-phase collection**: Each phase collects independently, parallel-safe
5. **Overflow table**: Locations too large to pack are stored in `LocationManager::locations`, referenced by an index
