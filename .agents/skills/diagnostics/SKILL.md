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
| `core/diag/Diagnostic.h` | `Diag` class, `DiagTag`, `DiagRelatedInfo`, `TextReplacement` |
| `core/diag/Diagnostic.cpp` | `make_diag`/`add_diag`, `print_diagnostics`, `color`, `to_string`, `Diag::format`, `Diag::ansi` |
| `core/diag/DiagSeverity.h` | `DiagSeverity` enum: `Error`, `Warning`, `Information`, `Hint` + `color`/`to_string` |
| `core/diag/Position.h` | `Position { line, character }` (0-based), `is_ahead`/`is_behind`/`is_equal`, `representation()` |
| `core/diag/Range.h` | `Range { start, end }`, `representation()` |
| `core/diag/Location.h` | `Location { Range range; std::string path; }` |
| `core/source/SourceLocation.h` | `SourceLocation` class wrapping an encoded `uint64_t`; `ZERO_LOC` |
| `core/source/LocationManager.h/.cpp` | Encodes/decodes source locations, owns file paths, packing/overflow logic |
| `compiler/ASTDiagnoser.h` | `ASTDiagnoser` — location-aware diagnoser extending `Diagnoser` (full helper set) |
| `compiler/ASTDiagnoser.cpp` | Location decoding + `dup_sym_error` / `unsatisfied_type_error` / `print_debug_location` |
| `compiler/ASTDiag.h` | Small `ASTDiag { std::string message; DiagSeverity severity; }` struct (unused legacy helper) |
| `parser/Parser.h` | `BasicParser : public ASTDiagnoser` — parser-local `error()/warning()/info()/hint()` at token position |
| `compiler/ASTProcessorOptions.h` | `ignore_errors`, `stop_on_file_error`, `verbose` — control error behavior |
| `compiler/ASTProcessor.h/.cpp` | Parallel pass driver; per-task diagnosers, `print_mutex`, `ConcurrentParsingState::has_errors` |
| `compiler/ASTCompiler.cpp` | Reports codegen diagnostics under `Declare` / `Compile` / `ExtDeclare` / `ExtCompile` tags |
| `compiler/Codegen.h` | `Codegen : public ASTDiagnoser` — codegen-side diagnostics |
| `compiler/CodegenOptions.h` | Codegen options that affect error behavior |
| `compiler/SanitizerOptions.h` | Sanitizer-related error handling |
| `preprocess/2c/2cASTVisitor.h` | `ToCAstVisitor : ... public ASTDiagnoser` — C-translation diagnostics (`2cTranslation`) |
| `compiler/symres/SymbolResolver.cpp` | `SymbolResolver : public ASTDiagnoser` — symres diagnostics (`SymRes`, `SymRes:*`) |
| `server/diagnostics/DiagnosticUtils.cpp` | `add_diagnostics` — compiler `Diag` → `lsp::Diagnostic` |
| `compiler/cbi/bindings/ASTDiagnoserCBI.h/.cpp` | CBI binding exposing `error` to compiler plugins |
| `compiler/cbi/bindings/ParserCBI.h/.cpp` | CBI binding `Parsererror_at` (parser error at a token) |
| `ast/base/ASTNode.h` | Base AST node — provides `encoded_location()` used for ranges |

## Data Model (`core/diag/`)

### `DiagSeverity`

`DiagSeverity` is a plain `enum class : int` (`core/diag/DiagSeverity.h:7-16`): `Error`, `Warning`,
`Information`, `Hint` (in that order). Spellings matter — `Information`, not `Info`; `Hint`, not
`Note`. Helpers `color(ostream&, DiagSeverity)` and `to_string(DiagSeverity)` return `"ERROR"`,
`"WARN"`, `"INFO"`, `"HINT"`.

`DiagTag : uint8_t` (`core/diag/Diagnostic.h:13-18`) is LSP-style metadata: `Unnecessary = 1`,
`Deprecated = 2`. `Diag::tags` is `std::optional<std::vector<DiagTag>>`; it is serializable but
**no compiler pass sets it**, and the LSP converter drops it.

### `Position` / `Range` / `Location`

- `Position { unsigned int line, character; }` (`core/diag/Position.h:7-35`). Both are **0-based**.
  `representation()` returns `"<line+1>:<character+1>"`. `is_ahead`/`is_behind`/`is_equal` compare
  line first, then character (`core/diag/Diagnostic.cpp:55-67`).
- `Range { Position start, end; }` (`core/diag/Range.h:7-19`). `representation()` returns a single
  `line:char` when start == end, otherwise `"line:char - line:char"`.
- `Location { Range range; std::string path; }` (`core/diag/Location.h:8-11`) — a range plus its
  absolute document path. This is the fully decoded, human-facing form.

### `DiagRelatedInfo` and `TextReplacement` (fix-its)

- `DiagRelatedInfo` (`core/diag/Diagnostic.h:20-26`): `Position location` + `std::string message`,
  stored in `Diag::relatedInformation` for "previous declaration here"-style context. Not populated by
  any pass and not propagated to LSP — `dup_sym_error` emits two diagnostics instead.
- `TextReplacement { Range range; std::string new_text; }` (`Diagnostic.h:28-32`): one edit that would
  fix a diagnostic, collected in `Diag::fixits_` (`:50-51`, "Non-serialized set of fixits"). Defined
  but never populated, and the LSP converter drops it, so quick fixes are not wired up. See
  [Quick Fix / Replacement Support](#quick-fix--replacement-support).

### `Diag`

`Diag` lives in `core/diag/Diagnostic.h` (not in `ASTDiag.h`) and carries a `Range` plus optional
metadata (`core/diag/Diagnostic.h:34-130`):

```cpp
class Diag {
public:
    Range range;                                  // start/end Position (line + character), 0-based

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

    static bool has_errors(std::vector<Diag>& diags);   // scans for DiagSeverity::Error
};
```

The `operator<<` overloads append to `message` only — they do **not** set severity, tags, or fix-its.
`Diag::has_errors(std::vector<Diag>&)` (`core/diag/Diagnostic.h:121-128`) is a static scan used when
you have a raw vector rather than a `Diagnoser`.

## The `Diagnoser` Base Class

`Diagnoser` (`core/diag/Diagnoser.h:12-99`) is the lowest-level collector. It has **no** `LocationManager`
and works in terms of `Position`/`Range`/explicit file paths. Everything else (`ASTDiagnoser`,
`SymbolResolver`, `Codegen`, `ToCAstVisitor`, `BasicParser`) derives from it.

```cpp
class Diagnoser {
public:
    unsigned int error_count = 0;              // incremented on each Error-severity diag
    std::vector<Diag> diagnostics;             // insertion order, never sorted

    static Diag make_diag(const chem::string_view& message, const chem::string_view& file_path,
                          const Position& start, const Position& end, DiagSeverity severity);
    void add_diag(Diag diag);                  // increments error_count if severity == Error
    bool has_errors();                         // error_count > 0
    Diag& empty_diagnostic(const chem::string_view& file_path, const Position& start,
                           const Position& end, DiagSeverity severity);   // append to this
    void diagnostic(const chem::string_view& message, const chem::string_view& file_path,
                    const Position& start, const Position& end, DiagSeverity severity);
    void reset_diagnostics();                  // error_count = 0; diagnostics.clear()

    static void print_diagnostics(std::vector<Diag>& diagnostics, const chem::string_view& path,
                                  const chem::string_view& tag);
    static void print_diagnostics(std::vector<Diag>& diagnostics, const chem::string_view& tag);
    void print_diagnostics(const chem::string_view& path, const chem::string_view& tag);
    void print_diagnostics(const chem::string_view& tag);
};
```

Key behaviors, verified in `core/diag/Diagnostic.cpp`:

- `make_diag` (`Diagnostic.cpp:10-20`) builds a `Diag` with `{range, severity, filePath.str(),
  message.str()}`.
- `add_diag` (`Diagnostic.cpp:22-27`) increments `error_count` **only when
  `diag.severity == DiagSeverity::Error`**; a missing optional does not count.
- `empty_diagnostic` (`Diagnoser.h:47-53`) also increments `error_count` for `Error` and returns a
  reference to the just-appended vector element so callers can `<<` into it.
- There is **no** `max_severity` field, **no** `has_warnings()`, and **no** `error_count()` method —
  `error_count` is a public field.
- **No sorting / dedup.** `print_diagnostics` emits diagnostics in insertion order.
- `reset_diagnostics()` clears both the vector and the count. On `ASTDiagnoser` this is surfaced as
  `reset_errors()` (`compiler/ASTDiagnoser.h:237-239`); the serial `SymbolResolver` uses
  `reset_errors()` after each file/pass.
- `[[deprecated]] void diagnostic(message, severity)` (`Diagnoser.h:65-66`) — the two-argument form
  without a location; implemented in `Diagnostic.cpp:29-31` with empty path and `Position{0,0}`.

## ASTDiagnoser

The `ASTDiagnoser` class is the primary location-aware collector. It extends the plain `Diagnoser`
(which owns the diagnostics and error count) and adds `SourceLocation` decoding via a `LocationManager`:

```cpp
class ASTDiagnoser : public Diagnoser {
public:
    LocationManager& loc_man;                    // decodes SourceLocation → path + Position range
    ASTDiagnoser(LocationManager& loc_man);

    void location_diagnostic(const chem::string_view& message, SourceLocation loc, DiagSeverity severity);
    Diag& empty_diagnostic(SourceLocation loc, DiagSeverity severity);  // empty Diag to append to
    Diag& error(SourceLocation); Diag& warning(SourceLocation); Diag& warn(SourceLocation); // warn == warning
    Diag& info(SourceLocation);  Diag& hint(SourceLocation);

    // message overloads: error(msg, loc), warn(msg, loc), info(msg, loc);
    // templated node overloads use node->encoded_location().
    void dup_sym_error(const chem::string_view& name, ASTNode* previous, ASTNode* new_node);
    void unsatisfied_type_error(Value* value, BaseType* type);
    // inherited: unsigned int error_count; std::vector<Diag> diagnostics;
    //            bool has_errors(); reset_diagnostics() (exposed as reset_errors())
};
```

> There is **no** `note()` method, **no** `suggest()`, **no** `undefined_symbol()`, no `max_severity`
> field, no `has_warnings()`, and no `error_count()` method — `error_count` is a public field on
> `Diagnoser`. Diagnostics with `Hint` severity are used for debug locations.

### Full `ASTDiagnoser` convenience API

All declared in `compiler/ASTDiagnoser.h`. The `SourceLocation`/node forms return a `Diag&` you append
to; the `(message, ...)` forms add immediately.

- **Empty-diag appenders (return `Diag&`)**: `error(SourceLocation)` `:81`, `warning`/`warn(SourceLocation)` `:65,73`,
  `info(SourceLocation)` `:57`, `hint(SourceLocation)` `:89`, `error<NodeT>(NodeT*)` `:116`,
  `warn<NodeT>(NodeT*)` `:106`, `info<NodeT>(NodeT*)` `:96`.
- **Message forms (return `void`)**: `error/warn/info(message, SourceLocation)` `:140,136,132`;
  `error/warn/info<NodeT>(message, NodeT*)` `:165,158,151`; `diagnostic(message, SourceLocation, DiagSeverity)` `:43`.
- **Two-location / two-node forms** emit **two** diagnostics with the same message
  (`:172-215`) — the compiler's stand-in for LSP `relatedInformation`.
- **Generic plumbing**: `location_diagnostic(msg, SourceLocation, DiagSeverity)` `:38`,
  `empty_diagnostic(SourceLocation, DiagSeverity)` `:51`,
  `diagnostic<NodeT>(msg, NodeT*, DiagSeverity)` `:144`.
- **Special helpers**: `print_debug_location(SourceLocation)` `:126` (prints a `Hint` `DEBUG_LOC` line),
  `dup_sym_error(name, previous, new_node)` `:220`, `unsatisfied_type_error(Value*, BaseType*)` `:232`,
  `reset_errors()` `:237`.

The node overloads are SFINAE-constrained with
`requires requires(NodeT n) { n.encoded_location(); }`, and the two-node overloads require it on both.

### How locations and ranges are computed

- `location_diagnostic` (`ASTDiagnoser.cpp:10-14`) calls `loc_man.getLocationPos(location)` →
  `{fileId, start, end}`, then `loc_man.getPathForFileId(pos.fileId)`, then
  `Diagnoser::diagnostic(message, filePath, start, end, severity)`. `empty_diagnostic` (`:16-20`) does
  the same but returns the empty `Diag&`. AST nodes expose an encoded range via
  `ASTNode::encoded_location()` (produced by the parser from tokens), so a diagnostic's `Range` is the
  node/token span.
- `print_debug_location` (`:22-25`) builds a `Hint` diagnostic and calls
  `Diag::format(std::cout, ..., "DEBUG_LOC")` using `diag.path_url.value()` — it prints, not collects.
- `dup_sym_error` (`:27-30`): `error(new_node) << "duplicate symbol being declared, symbol '…' already
  exists"` + `warn(previous) << "symbol has a conflict"`.
- `unsatisfied_type_error` (`:32-39`): `"value with type 'X' does not satisfy type 'Y'"`, or
  `"value does not satisfy type 'Y'"` when untyped. (The `TypeVerify.cpp:234` free function
  `unsatisfied_type_err` is what the type checker actually uses; this member is a second copy.)

### Parser ranges and parser-local diagnostic helpers

`BasicParser : public ASTDiagnoser` (`parser/Parser.h:50`) carries `file_id` + the current `token`.
`loc(start, end)` (`Parser.cpp:39-41`) calls `addLocation(file_id, start.line, start.character,
end.line, end.character)`; `loc_single(pos, length)` (`:53-55`) uses `pos.line` for both ends with
`pos.character + length`. `loc(token)` spans `token->position` .. `token->position +
token->value.size()`, and `get_file_path()` (`Parser.cpp:106+`) resolves `file_id` through the
`LocationManager`. On top of that, `BasicParser` adds a token-position layer over `ASTDiagnoser`
(`parser/Parser.h:249-349`): `make_diag`,
`empty_diagnostic(start[, end], severity)`, `diagnostic(severity)`, the empty appenders
`error()/warning()/info()/hint()`, the message forms `error(msg)/warning(msg)/info(msg)/hint(msg)`,
`unexpected_error(msg)` (appends `", got '<token>'"`), `error(msg, Position)`, and
`error(SourceLocation)` (forwards to `ASTDiagnoser`). That is why parser code reads
`parser.error("expected a ')' after …")` or `parser.error() << "expected … " << id`
(`parser/statements/Typealias.cpp:15-81`, `parser/values/StructValue.cpp:13-21`,
`parser/structures/Struct.cpp:251`), and why `Parsererror_at`
(`compiler/cbi/bindings/ParserCBI.cpp:43-45`) forwards to `parser->error(*view, token->position)`.

## `ASTDiag.h`

`ASTDiag.h` only declares a small value type — there are no predefined message helpers here:

```cpp
// compiler/ASTDiag.h
struct ASTDiag {
    std::string message;
    DiagSeverity severity;
};
```

It is a leftover/legacy helper: no production code constructs an `ASTDiag` (grep finds references
only in the header). All diagnostics are created through `ASTDiagnoser`
(`error(...)`, `warn(...)`, `info(...)`, `hint(...)`) or the `Diagnoser` base API.

## Severity Levels

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

> ⚠️ **`ZERO_LOC` vs `encodeFile`.** The comment claims fileId `0` is invalid, but `encodeFile`
> (`LocationManager.cpp:6-16`) gives id `0` to the **first** file. A valid location whose fields are
> all `0` therefore encodes to exactly `ZERO_LOC`; do not rely on `isValid()` to mean "unknown".

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

The shifts are derived, not hand-written (`LocationManager.h:78-80`):
`FILE_ID_SHIFT_BITS = 18+12+11+12 = 53`, `LINE_START_SHIFT_BITS = 35`, `CHAR_START_SHIFT_BITS = 23`.
Storing the line end as an **offset** is what keeps the range in 63 bits: `lineEnd` is decoded as
`lineStart + offset` (`LocationManager.cpp:107`). The encoding is an in-memory `uint64_t` and is never
written to disk or across processes, so there are **no explicit endianness concerns**; shifts/masks
are host-standard. There is also no explicit `static_assert` that the bit widths sum to ≤ 63 — they do,
by hand-keeping.

Decode with `getLocation(SourceLocation)` (returns `LocationData { fileId, lineStart, charStart,
lineEnd, charEnd }`) or `getLocationPos(SourceLocation)` (returns `{ fileId, Position start,
Position end }`). `getLineStartFast()` skips the full decode when only the line is needed.

> ⚠️ **`addLocation` filter bug.** The range check (`LocationManager.cpp:49-55`) uses
> `(lineEnd - lineEnd) <= MAX_LINE_END_OFFSET`, always true, so it never rejects an oversized span.
> The `#ifdef DEBUG` guard (`:57-59`) does throw, but release builds silently truncate. Fix the
> predicate to `(lineEnd - lineStart) <= MAX_LINE_END_OFFSET`.

### LocationManager

```cpp
class LocationManager {
    std::vector<LocationData> locations;              // overflow table for ranges too large to pack
    tsl::ordered_map<std::string, bool> file_paths;   // file id → path (insertion order)
    std::mutex file_mutex;                            // guards encodeFile/encodeExistingFile
    std::mutex location_mutex;                        // guards the overflow table only

    unsigned int encodeFile(const std::string& filePath);      // new or existing id
    int encodeExistingFile(const std::string& filePath);       // id, or -1 if never encoded
    std::string_view getPathForFileId(unsigned int fileId);    // path for id

    uint64_t addLocation(uint32_t fileId, uint32_t lineStart, uint32_t charStart,
                         uint32_t lineEnd, uint32_t charEnd);
    LocationData getLocation(uint64_t data) const;
    LocationData getLocation(SourceLocation loc) const;
    LocationPosData getLocationPos(SourceLocation loc) const;
    unsigned int getFileId(SourceLocation loc) const;

    uint32_t getLineStartFast(SourceLocation data);
    std::string formatLocation(SourceLocation location);
};
```

**Ownership.** A single `LocationManager` is owned by the enclosing build/processor and referenced by
every diagnoser/parser in that scope:

- `ASTProcessor` holds `LocationManager& loc_man` (`compiler/ASTProcessor.h:91`) and passes it to
  every `ASTDiagnoser` and `SymbolResolver`.
- `GlobalInterpretScope`/`InterpretScope` carry `loc_man` (`ast/base/GlobalInterpretScope.cpp:18`).
- The LSP server owns one session-wide `LocationManager loc_man` (`server/WorkspaceManager.h`),
  shared by all files, and every server analyzer decodes through it.
- `encodeFile` deduplicates: encoding the same path twice returns the same id
  (`LocationManager.cpp:6-16`). `getPathForFileId(id)` is O(1) random access by iterator arithmetic on
  the `tsl::ordered_map` — valid only while the map is alive and the id was actually issued.

**Multi-file positions.** A `SourceLocation` encodes the **file id**, so a single
`ASTDiagnoser`/`Diag` can point at any file in the build. During a pass, `location_diagnostic`
resolves the correct path per-diagnostic, so one diagnoser can emit errors for several files
(e.g. duplicate symbols across modules). Related cross-file ranges are what LSP analyzers resolve
via `loc_man.getLocationPos` + `getPathForFileId`.

Encode/decode round-trip:

```cpp
uint64_t loc = loc_man.addLocation(fileId, lineStart, charStart, lineEnd, charEnd);
SourceLocation sloc(loc);
auto data = loc_man.getLocation(sloc);        // LocationData
auto pos  = loc_man.getLocationPos(sloc);     // LocationPosData { fileId, Position start, Position end }
```

`formatLocation` (`LocationManager.cpp:28-41`) is a debug/console helper used by
`LabBuildCompiler`/`ASTProcessor` error paths; its output is `<path><line>:<char>:` — **0-based**,
with no separator between path and line and a trailing colon. It is not used for user-facing output
(`Diag::ansi` is).

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
struct TypeVerifyFileResult {                 // ASTProcessor.cpp:754-757
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

This is the critical part of the diagnostics design. Files and passes run in parallel, but
`Diagnoser`/`Diag` are **not** thread-safe. The pattern is:

1. **One diagnoser per task.** Each parallel task makes its own
   `ASTDiagnoser(processor->loc_man)` or per-file visitor, so no two threads touch the same
   `error_count`/`diagnostics`. `type_verify_file_task` (`ASTProcessor.cpp:759-773`) runs
   `type_verify`, then `result.has_errors = diagnoser.has_errors(); result.diagnostics =
   std::move(diagnoser.diagnostics);`. `link_sig_file_task`, `gen_inst_file_task`, `link_body_task`
   etc. have the same shape and return `{bool, std::vector<Diag>}`.
2. **Move, don't copy.** Diagnostics are `std::move`d out of the local diagnoser into the result, so
   the task-local object is left empty and ownership transfers to the joining thread.
3. **Print under a mutex.** After `future.get()`, the main thread takes
   `std::lock_guard<std::mutex> guard(print_mutex)` (`compiler/ASTProcessor.h:122`) before
   `Diagnoser::print_diagnostics(...)` — e.g. `ASTProcessor.cpp:504-506,538-540,586-588,607-609`
   (symres), `:793-797` (type verify), and `:1144` (codegen module path).
4. **Aggregate a bool, not a counter.** `ConcurrentParsingState` (`ASTProcessor.h:56-75`) holds an
   `std::atomic_bool has_errors` (`set_has_errors`/`get_has_errors`) plus an outstanding-task counter
   and join promise. `ASTProcessor::empty_diags`/`print_results` (`ASTProcessor.cpp:71-86`) print
   lexer/parser results.
5. **Serial phases** (top-level declare, after-link-signature) reuse one `resolver` and call
   `reset_errors()` between files, reusing `error_count` rather than accumulating it
   (`ASTProcessor.cpp:482-486`, `:565-569`).

`LocationManager` is itself thread-safe for the two operations that can race: `encodeFile` /
`encodeExistingFile` take `file_mutex` (`LocationManager.cpp:7,19`), and the overflow-table insert
in `addLocation` takes `location_mutex` (`LocationManager.cpp:69`). The common packed case is
lock-free.

## Diagnostic Flow Through the Pipeline

Each pass tags its output with the phase string passed to `print_diagnostics`, which becomes the
`[<tag>]` prefix (e.g. `[TypeCheck] error: …`). Pipeline order and real call sites:

| Phase | Tag | Call site |
|-------|-----|-----------|
| Lexer | `"Lexer"` | `ASTProcessor.cpp:78`, `LabBuildCompiler.cpp:2413,2520` |
| Parser | `"Parser"` | `ASTProcessor.cpp:79`, `LabBuildCompiler.cpp:2414,2521` |
| Top-level declare | `"SymRes:declare"` | `ASTProcessor.cpp:228` |
| Link signature | `"SymRes:link_sig"` | `ASTProcessor.cpp:246,505` |
| Generic instantiation | `"SymRes:gen_inst"` | `ASTProcessor.cpp:269,539,587` |
| After link signature | `"SymRes:after_link_sig"` | `ASTProcessor.cpp:351` |
| Link body | `"SymRes:link"` / `"SymRes:link_seq"` | `ASTProcessor.cpp:367,384,608` |
| Type verify | `"TypeCheck"` | `ASTProcessor.cpp:796` |
| Codegen (LLVM) | `"Declare"`/`"Compile"`/`"ExtDeclare"`/`"ExtCompile"` | `ASTCompiler.cpp:24,44,64,84,112,133` |
| 2c translation | `"2cTranslation"` | `ASTProcessor.cpp:1563,1585,1607` |
| `.mod` file | `"Lexer"` / `"Parser"` | `server/mod_file/Importer.cpp`; `LabBuildCompiler.cpp:2413-2414` |
| LSP publish | (no tag; LSP `Diagnostic`) | `server/diagnostics/DiagnosticUtils.cpp:5` |

### Error handling controls

`ASTProcessorOptions` (`compiler/ASTProcessorOptions.h`) governs how far a build proceeds:

- `ignore_errors = false` (line 42) — when true, passes do **not** abort after reporting; diagnostics
  are still collected.
- `stop_on_file_error = true` (line 58) — when true, the pass aborts after the current phase
  once any file has reported errors, "printing less errors to console". The parallel loops
  check `if(options->stop_on_file_error) return 1;` **after** draining every future
  (`join_all(futures)`) — a pending task may never be abandoned, because tasks reference the
  caller's stack objects (`ASTProcessor`, allocators); see
  [`lang/docs/lab-build-crash-investigation.md`](../../docs/lab-build-crash-investigation.md).
- `verbose` (line 32) — only affects progress/`[lab]`/benchmark prints; it does **not** filter or
  change diagnostics. `-v` does not suppress hints either (the "omit hints unless verbose" idea is a
  future improvement, not current behavior).

### Build system surfacing and exit codes

- Parse failure inside a lab file: `LabBuildCompiler.cpp:2767-2772` checks
  `state.get_has_errors()` and returns `nullptr` (with a verbose-only `[lab]` message).
- Codegen failure: `LabBuildCompiler.cpp:1192-1195` checks `gen.has_errors()`, prints
  `couldn't perform job due to errors during code generation` to `stderr`, and returns `1`.
- `.mod` diagnostics are printed in `LabBuildCompiler.cpp:2413-2414` / `:2520-2521`.
- The CLI returns a non-zero code on missing input (`core/main/CompilerMain.cpp:597-599`) and from
  the job-loop failures (the many `return 1`/`return 2` sites in `LabBuildCompiler.cpp`). There is no
  single central "exit code = error_count"; errors surface as returned `1`/`2` from
  `LabBuildCompiler` run functions, and type/symres errors abort before codegen.

## Error Messages

### Format

`Diag::ansi()` (`core/diag/Diagnostic.cpp:105-124`) renders a single-line, ANSI-coloured diagnostic
(line/column are 1-based; `path` is the file path or, if present, `Diag::path_url`):

```
[<tag>] error: <message> at <path>:<line>:<column>
```

`severity` is optional: if absent, `ansi` prints **no** `error:`/`warn:` word — just
`[<tag>] <message> at <path>:<line>:<column>`. Warnings render `[<tag>] warn: …`, info
`[<tag>] info: …`, and hints `[<tag>] hint: …`. `Diag::format()` is the non-coloured variant
(`[<tag>] <message> at <path>:<range>`) used by `print_debug_location` and a few debug sites.
There is no multi-line snippet/caret rendering and no `= note:` continuation lines — related context
is meant to be carried in `Diag::relatedInformation` (currently unused). `Range::representation()`
is only reached by `format()`, not `ansi()`.

### ANSI colors

Colors come from the vendored `rang.hpp` (`core/diag/Diagnostic.cpp`) and are emitted by both
`color()` (`Diagnostic.cpp:69-84`) and `Diag::ansi` (`:105-124`): `Error` red, `Warning` yellow,
`Information` gray, `Hint` cyan. `print_diagnostics` (`Diagnostic.cpp:33-53`) routes **`Error` to
`std::cerr`** and everything else to `std::cout`, flushing both at the end; the `tag` is emitted
verbatim inside `[...]`.

> There is **no source-line excerpt and no caret underline**. Diagnostics are one line each; `Diag`
> has only a `Range` and `print_diagnostics` has no file-loading step.

### Error limit

There is no per-diagnostic cap. The only "limit" is `ASTProcessorOptions::stop_on_file_error`, which
short-circuits a pass after the first file with errors to reduce console noise; `ignore_errors`
bypasses the abort so more errors accumulate.

### Common Error Types

| Error Pattern | Source | Real message / example |
|---------------|--------|------------------------|
| Duplicate symbol | `ASTDiagnoser::dup_sym_error` (`ASTDiagnoser.cpp:27-30`), `NodeSymbolDeclarer.h:53` | `duplicate symbol being declared, symbol 'x' already exists` + `symbol has a conflict`; or `symbol with name 'x' already exists` |
| Symbol not found | `LinkSignature.cpp:130,253` | `unresolved variable identifier 'foo' not found` |
| Unsatisfied type | `TypeVerify.cpp:234` (`unsatisfied_type_err`) | `value with type 'float' does not satisfy type 'int'` |
| Assignment | `TypeVerify.cpp:1539,1557` | `Expression is not assignable` / `cannot assign to a non mutable value` |
| Struct init | `LinkSignature.cpp:501` | `struct with name 'X' has a constructor, use @direct_init to allow direct initialization` |
| Uninitialized variable | `TypeVerify.cpp:109,354` | `use of uninitialized variable 'x' before it is initialized (use of)` |
| Destructible index | `TypeVerify.cpp:354` | `index operator on a destructible type is not allowed, use \`&raw\` to take a pointer to the element instead` |
| Unknown annotation | `parser/statements/AnnotationMacro.cpp:28` | `unknown annotation found '@foo'` (parse error) |
| Runtime outside function | `LinkSignature.cpp:414` (`RUNTIME_EVAL_ERR`) | `cannot evaluate at runtime outside function body` |
| Non-exported type | `LinkSignature.cpp:144` | `non exported type being used in a public type, please use 'public' or 'protected' to expose it` |
| Unsafe deref | `SymResLinkBody.cpp:2457` | `de-referencing a pointer in safe context is prohibited` (warning) |
| Malformed source | `parser/structures/Struct.cpp:251` | `expected a closing brace '}' for [<thing>]` |

## Plugin Diagnostics

Compiler plugins report diagnostics through the CBI binding in
`compiler/cbi/bindings/ASTDiagnoserCBI.h/.cpp` — a single no-mangle symbol
`void ASTDiagnosererror(ASTDiagnoser* diagnoser, chem::string_view* msg, uint64_t loc)` that calls
`diagnoser->error(*msg, loc)`. Registered in `CBI.cpp:464-465,491` as
`compiler_ASTDiagnosererror` under the `ASTDiagnoser` interface. The Chemical side is
`lang/libs/compiler/src/ASTDiagnoser.ch`: `func error(&self, msg : &std::string_view, loc : ubigint)`.
CBI exposes it to plugins as `compiler_ASTDiagnosererror`; there is no
`cbi_report_warning/info/note` family. Plugins can also report at a token through the parser binding
`Parsererror_at` (`compiler/cbi/bindings/ParserCBI.cpp:43-45`, registered as
`compiler_Parsererror_at` in `CBI.cpp:148`), surfaced in
`lang/libs/compiler/src/Parser.ch:25,75` (`func error_at(&self, msg, token)`).

> Only `Error` severity crosses the plugin boundary. A plugin cannot currently emit warnings/hints
> through CBI, and plugin diagnostics carry a raw `ubigint` encoded location (obtained from an AST
> node), not a `Diag` object.

## LSP Integration

The LSP server converts compiler diagnostics to `lsp::Diagnostic` in
`server/diagnostics/DiagnosticUtils.cpp:5-16` (`add_diagnostics`): it maps `diag.range` to an
`lsp::Range`, copies `diag.message`, and integer-casts `diag.severity.value()` to
`lsp::DiagnosticSeverity`. Key points (cross-link: `.agents/skills/lsp_server/SKILL.md` §4.1, §6):

- `Diag::range` maps **directly** to `lsp::Range` (both 0-based; `Diag::ansi` adds `+1` only for
  display). Severity is a raw integer cast whose `Error/Warning/Information/Hint` order matches
  `lsp::DiagnosticSeverity` (1..4).
- The converter assumes `diag.severity.has_value()` and **unwraps it without a check** — a `Diag` with
  no severity is UB here. All diagnostics from `ASTDiagnoser` and parser helpers do set severity.
- `relatedInformation`, `tags`, and `fixits_` are **dropped**.
- Sources fed into `add_diagnostics`: lexer (`LexResult::diags`), parser (`parser.diagnostics`), and
  `SymbolResolver` (`resolver.diagnostics`), accumulated in `process_file`
  (`server/LspSemanticTokens.cpp:990-997`). Type verification does **not** run in the LSP.
- Publishing uses `lsp::notifications::TextDocument_PublishDiagnostics` via `publish_diagnostics`
  (`LspSemanticTokens.cpp:1266`), serialized by `publish_diagnostics_mutex` with an
  `std::async` cancel flag. Build failures are reported separately through the custom
  `chemical/buildStatus` notification (`WorkspaceManager::report_build_failure`,
  `WorkspaceManager.cpp:596`) so they do not clobber per-URI diagnostics.
- Ranges for navigation features (hover, go-to-definition) come from the same
  `LocationManager::getLocationPos` decode, so the location layer is shared between diagnostics and
  the rest of the IDE surface.

## Quick Fix / Replacement Support

The data structures exist but the pipeline does not use them yet: `TextReplacement`/`Diag::fixits_`
(`Diagnostic.h:28-32,51`), `DiagTag::Unnecessary`/`Deprecated`/`Diag::tags` (`:13-18,54`),
`DiagRelatedInfo`/`Diag::relatedInformation` (`:20-26,58`). What is used instead: related context is
emitted as **separate diagnostics** (`dup_sym_error` plus the two-node/two-location overloads), and
deprecation is a node flag (`set_deprecated` via `@deprecated` —
`.agents/skills/annotations/SKILL.md`), not a `DiagTag`.

To wire quick fixes end-to-end: populate `Diag::fixits_`, translate `TextReplacement` → `lsp::TextEdit`
and `tags`/`relatedInformation` in `add_diagnostics`, and register a `textDocument/codeAction` handler
in `LSPMain.cpp` (none exists — no `codeAction` capability is advertised).

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

> Current state: (1) is not done — `diagnostics` is an unordered insertion-order vector and
> `print_diagnostics` iterates it directly. (3) is actually done via `rang.hpp` colors in
> `Diag::ansi`. (5) is not done. (7) is partially achieved by `ignore_errors` + `stop_on_file_error`.
> The rest remain unstarted.

## Adding a New Diagnostic or Error Message

Checklist and conventions:

1. **Pick the right API.** Most passes have an `ASTDiagnoser&` and an AST node: prefer
   `diagnoser.error(node) << "message " << value;` (append) or `diagnoser.error("message", node)`.
   In the parser use `parser.error("...")` / `parser.error() << ...`. Use `warn` only for non-fatal
   issues. There is no `note`/`suggest`.
2. **Attach a real location.** Never use the two-arg deprecated `diagnostic(message, severity)`
   (defaults to `{0,0}` + empty path). Use a node (`encoded_location()`), a `SourceLocation`, or a
   parser `Position`.
3. **Message style.** Lowercase, descriptive, condition-first: `"unresolved variable identifier 'x'
   not found"`, `"cannot assign to a non mutable value"`, `"expected a ')' after …"`. Built by
   streaming `operator<<`; include offending names/types in single quotes via `representation()`.
4. **Choose severity deliberately.** `Error` aborts (unless `ignore_errors`); warnings and below do
   not, and only `Error` increments `error_count`.
5. **Respect threading.** Inside a per-file parallel task use the task-local diagnoser; never emit into
   a shared one from multiple threads. Report via the result struct; the join path prints under
   `print_mutex`.
6. **Add a phase tag** for a new pass (the string passed to `print_diagnostics`, e.g. `SymRes:*`,
   `TypeCheck`, `Declare`/`Compile`, `2cTranslation`).
7. **Test** with `lang/tests/negative/` (asserts exit code + stderr, see
   `.agents/skills/testing/SKILL.md`) or an isolated `lang/compiled/temp.ch`.
8. **CBI enum sync** is not needed for a plain diagnostic, but mirror any new CBI-exposed
   severity/`DiagTag`/`DiagSeverity` in the `.ch` binding (AGENTS.md enum-sync rule).

Conventions quick list: lowercase messages, single-quoted identifiers, `operator<<` composition,
attach a node/location, lowest correct severity, parallel-safe collection, phase tag.

## Gotchas

- **Location validity.** `isValid()` is `encoded != 0`, but file id `0` is the legitimate first-file
  id (see `ZERO_LOC`). `ASTDiagnoser` does not guard a zero location: it decodes to `fileId 0` and
  calls `getPathForFileId(0)`, which is UB if no file was ever encoded.
- **Dangling file paths.** `getPathForFileId` returns a `string_view` into
  `LocationManager::file_paths`; it dangles if the manager is destroyed or the map mutates. In the LSP
  a single `loc_man` lives for the session, and cached locations go stale when a file is re-lexed
  (LSP skill §9.1).
- **No dedup, no sort.** Expect duplicate and out-of-order messages, especially across parallel files.
- **`error_count` is a field, not a method.** It counts only `Error` severity, including empty
  diagnostics created via `error(loc)` even with no appended message.
- **Warnings do not fail the build.** There is **no** Chemical warnings-as-errors / `-Werror` (the
  `-Werror=` flags in `CMakeLists.txt:76,91` are for the C++ compiler). A warnings-only pass returns
  `has_errors() == false` and continues. Emit `Error` if it must be fatal.
- **Severity-optional `Diag`.** Hand-built `Diag`s may have `severity == std::nullopt`; they print
  with no severity word and go to `stdout`. The LSP converter unwraps `.value()` unconditionally, so
  always set severity for anything reaching the LSP.
- **`relatedInformation`/`tags`/`fixits_` are dead ends today** — setting them changes no output until
  the consumers are extended.
- **`addLocation` overflow is unchecked in release** (see the filter bug above). Keep line < 262144,
  char < 4096, span < 2048 lines or the location silently truncates.
- **Interpreter:** `GlobalInterpretScope` derives from `ASTDiagnoser`
  (`ast/base/GlobalInterpretScope.cpp:18`), so `InterpretScope` reports via
  `global->getASTDiagnoser().warn(location)` (`ast/base/InterpretScope.h:276`) using the same encoding.

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

### Checking, counting, and inspecting

```cpp
if(diagnoser.has_errors()) { return; }             // skip codegen when error_count > 0
unsigned int err_count = diagnoser.error_count;    // public field, not a method
for(auto& d : diagnoser.diagnostics) {
    d.ansi(std::cerr, chem::string_view("<path>"), "MyTag") << '\n';   // colored, 1-based
    // or: d.format(std::cout, path, "MyTag");                        // plain, range form
}
```

### Isolating a failing file

Copy the failing construct into `lang/compiled/temp.ch` and compile it directly with
`Compiler`/`TCCCompiler` (see AGENTS.md "Debugging: Isolating a Single Test Case"). The emitted
diagnostic tag tells you which pass produced the message, and `--mode`/`-v` control progress output,
not the diagnostics themselves.

## Performance Considerations

1. **Owned messages**: `Diag::message` is a `std::string`; file paths are deduplicated in `LocationManager::file_paths`
2. **Move semantics**: Diagnostics are moved (not copied) out of phase-local diagnosers
3. **Per-file abort**: `ASTProcessorOptions::stop_on_file_error` decides whether a module aborts after a phase reports errors
4. **Per-phase collection**: Each phase collects independently, parallel-safe
5. **Overflow table**: Locations too large to pack are stored in `LocationManager::locations`, referenced by an index

Additional notes: only oversized locations take `location_mutex`; packed ones are lock-free
(`LocationManager.cpp:47-75`). `encodeFile` stores each path once, printing is serialized under
`print_mutex`, and `getLineStartFast` skips the full 5-field decode.

## Cross-Links

- **LSP Server** (`.agents/skills/lsp_server/SKILL.md`) — `add_diagnostics`, publishing, `Diag`→range.
- **Parser Internals** (`.agents/skills/parser_internals/SKILL.md`) — `BasicParser`, `loc()`, error recovery.
- **Type Verification** (`.agents/skills/type_verification/SKILL.md`) — `unsatisfied_type_err`, `TypeCheck` messages.
- **Symbol Resolution** (`.agents/skills/symres/SKILL.md`) — `SymbolResolver` (an `ASTDiagnoser`), `dup_sym_error`.
- **Annotations** (`.agents/skills/annotations/SKILL.md`) — `@deprecated`, annotation parse errors.
- **Compiler Bindings / CBI** (`.agents/skills/compiler_bindings/SKILL.md`, `.agents/skills/cbi_plugin_api/SKILL.md`) — `ASTDiagnosererror`, `Parsererror_at`, enum-sync.
- **Testing** (`.agents/skills/testing/SKILL.md`) — negative tests and `@test` dispatch.
- **Build System** (`.agents/skills/build_system/SKILL.md`) — how jobs surface errors and exit codes.
