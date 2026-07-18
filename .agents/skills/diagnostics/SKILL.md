---
name: Diagnostics and Error Reporting
description: Comprehensive guide to the Chemical compiler's diagnostic system — how errors, warnings, and informational messages are collected, reported, and formatted.
---

# Diagnostics and Error Reporting

The Chemical compiler has a sophisticated diagnostics system that captures errors, warnings, and informational messages during all compilation phases. Diagnostics are collected in `ASTDiagnoser` objects and reported through the `ASTDiag` system.

## Architecture

### Key Files

| File | Purpose |
|------|---------|
| `compiler/ASTDiagnoser.h` | Core `ASTDiagnoser` class — collects and manages diagnostics |
| `compiler/ASTDiagnoser.cpp` | Implementation — source location, message formatting |
| `compiler/ASTDiag.h` | Diagnostic types and message definitions |
| `compiler/CodegenOptions.h` | Codegen options that affect error behavior |
| `compiler/SanitizerOptions.h` | Sanitizer-related error handling |
| `ast/base/ASTNode.h` | Base AST node — provides `encoded_location()` |
| `compiler/cbi/bindings/ASTDiagnoserCBI.h/.cpp` | CBI bindings for diagnostics (for compiler plugins) |

## ASTDiagnoser

The `ASTDiagnoser` class is the primary diagnostic collector:

```cpp
class ASTDiagnoser {
public:
    LocationManager& loc_man;                    // Source location manager
    std::vector<Diag> diagnostics;               // Collected diagnostics
    Severity max_severity;                       // Highest severity seen
    
    // Methods:
    void error(SourceLocation loc, const chem::string_view& message);
    void warning(SourceLocation loc, const chem::string_view& message);
    void info(SourceLocation loc, const chem::string_view& message);
    void note(SourceLocation loc, const chem::string_view& message);
    
    bool has_errors() const;                     // Check if any errors were collected
    bool has_warnings() const;                   // Check if any warnings were collected
    size_t error_count() const;                  // Count of errors
    
    void merge(ASTDiagnoser& other);             // Merge diagnostics from another diagnoser
};
```

### Diagnostic Structure

```cpp
struct Diag {
    Severity severity;          // Error, Warning, Info, Note
    SourceLocation location;    // Source position
    chem::string message;       // Human-readable message
    
    // Optional:
    chem::string snippet;       // Source code snippet
    chem::string hint;          // Suggestion fix
};
```

### Severity Levels

```
Error   → Compilation fails (has_errors() == true → no codegen)
Warning → Compilation succeeds, warning printed
Info    → Informational message
Note    → Additional context for another diagnostic
```

## Source Location System

### SourceLocation

`SourceLocation` is an encoded 64-bit value that packs:

```
Bits 0-31: Position offset in file
Bits 32-47: Line number
Bits 48-63: File ID (index in LocationManager)
```

### LocationManager

```cpp
class LocationManager {
    std::vector<SourceFile> files;       // All source files
    std::unordered_map<uint32_t, chem::string> file_paths;  // File ID → path
    
    // Get human-readable location string
    chem::string format(SourceLocation loc);
    
    // Get source line at location
    chem::string_view get_line(SourceLocation loc);
};
```

### Encoded Location

```cpp
// Encoding:
SourceLocation encode_location(uint32_t file_id, uint32_t line, uint32_t column);

// Decoding:
uint32_t file_id = loc >> 32;
uint32_t line = (loc >> 16) & 0xFFFF;
uint32_t column = loc & 0xFFFF;
```

## Diagnostic Collection Flow

### Per-Phase Collection

Each compilation phase collects diagnostics independently:

```cpp
// In SymbolResolver:
struct SymResSignatureResult {
    ASTDiagnoser diagnoser;  // Own diagnoser for this phase
    // ... results
};

// After phase completes:
main_diagnoser.merge(phaseResult.diagnoser);
```

### Merging Flow

```cpp
void ASTDiagnoser::merge(ASTDiagnoser& other) {
    diagnostics.insert(
        diagnostics.end(),
        std::make_move_iterator(other.diagnostics.begin()),
        std::make_move_iterator(other.diagnostics.end())
    );
    other.diagnostics.clear();
    
    if(other.max_severity > max_severity) {
        max_severity = other.max_severity;
    }
}
```

### Parallel Safety

In parallel phases (SymResLinkBody, GenericInstantiation pass), each thread owns its own `ASTDiagnoser`:

```cpp
// Each file gets its own SymResLinkBody with its own diagnoser
auto bodyVisitor = SymResLinkBody(resolver);
// ... resolve ...
mainDiagnoser.merge(bodyVisitor.diagnoser);  // Merge after completion
```

## Error Messages

### Format

Standard error format:

```
file.ch:line:column: error: message
    |
 N | source code line
    | ^~~~~~~~~~~~~~~~ highlight
    |
    = note: additional context
```

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

Compiler plugins can report diagnostics via the CBI binding:

```cpp
// In ASTDiagnoserCBI.cpp:
void cbi_report_error(void* diagnoser, const char* file, int line, int col, const char* message) {
    auto* diag = static_cast<ASTDiagnoser*>(diagnoser);
    auto loc = diag->loc_man.make_location(file, line, col);
    diag->error(loc, message);
}
```

Plugins can also use:
- `cbi_report_warning(diagnoser, ...)`
- `cbi_report_info(diagnoser, ...)`
- `cbi_report_note(diagnoser, ...)`

## ASTDiag.h

`ASTDiag.h` contains predefined diagnostic messages and helper functions:

```cpp
// Example patterns:
void report_undefined_symbol(ASTDiagnoser& diag, SourceLocation loc, const chem::string_view& name);
void report_type_mismatch(ASTDiagnoser& diag, SourceLocation loc, BaseType* expected, BaseType* actual);
void report_cannot_move_value(ASTDiagnoser& diag, SourceLocation loc, Value* val);
```

These helpers standardize error messages and ensure consistent formatting.

## Print/Buffer Strategy

### Current Approach

Diagnostics are printed immediately as they are collected in some paths, but the modern approach collects them and prints them after each phase:

```cpp
// After phase:
if(diagnoser.has_errors()) {
    for(auto& diag : diagnoser.diagnostics) {
        print_diagnostic(diag);
    }
}
```

### Future Improvements

1. **Batch and sort** diagnostics by file/line/column
2. **Deduplicate** repeated errors
3. **Colored output** based on severity
4. **JSON output** for IDE integration
5. **Compressed output** — omit notes unless verbose (-v)
6. **Diagnostic groups** — group related errors together
7. **Error recovery** — continue compilation to find more errors
8. **Suppression** — allow suppressing specific warnings

## Debugging Diagnostics

### Adding a Diagnostic

```cpp
// In any AST visitor:
diagnoser.error(node->encoded_location(), "this feature is not yet implemented");
// Or:
diagnoser.warning(loc, "this operation is deprecated, use 'new_thing' instead");
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
size_t err_count = diagnoser.error_count();
if(err_count > MAX_ERRORS) {
    // Early exit to avoid excessive error messages
}
```

## Performance Considerations

1. **Lazy formatting**: Messages are stored as `chem::string` views until needed
2. **Move semantics**: Diagnostics are moved (not copied) between phases
3. **Max error limit**: Stop after N errors to avoid flooding output
4. **Per-phase collection**: Each phase collects independently, parallel-safe
5. **File-backed locations**: Source locations reference files in `LocationManager`, no string copying for file paths
