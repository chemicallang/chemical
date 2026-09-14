---
name: Compiler Plugin API (CBI)
description: Comprehensive guide to the Chemical Compiler Binding Interface (CBI) — how compiler plugins are built, registered, and integrated into the compilation pipeline.
---

# Compiler Plugin API (CBI)

The Chemical Compiler Binding Interface (CBI) allows external Chemical code to hook into the compilation process. Plugins are compiled via TinyCC JIT at build time and can intercept lexing, parsing, symbol resolution, and codegen.

## Architecture

### How Plugins Work

1. **Plugin declaration**: A `build.lab` script implements `build(ctx : *mut BuildContext, user_job : *mut LabJob) : *mut Module` and creates a CBI job with `ctx.build_cbi(name)` (`LabJobType::CBI`)
2. **Plugin compilation**: The plugin's Chemical source is compiled to C, then JIT-compiled via TinyCC like any other module
3. **Function indexing**: The build script registers each plugin entrypoint with `ctx.index_cbi_fn(...)` (or `ctx.index_def_cbi_fn(...)`), recording its name and its `CBIFunctionType` in the binder
4. **Hook execution**: During compilation the binder looks up the function by `(key, CBIFunctionType)` and calls it — e.g. `ParseMacroNode`, `SymResNode`, `ReplacementNode`

### Key Files

| File | Purpose |
|------|---------|
| `lang/libs/compiler/` | Compiler API bindings — the API that plugins call (interfaces declared in `chemical.mod`) |
| `lang/libs/compiler/src/ASTBuilder.ch` | AST node construction — create any Chemical AST node |
| `lang/libs/compiler/src/Lexer.ch` | Lexer bindings — tokenize Chemical source |
| `lang/libs/compiler/src/Parser.ch` | Parser bindings — parse Chemical source |
| `lang/libs/compiler/src/SymbolResolver.ch` | Symbol resolver bindings |
| `lang/libs/compiler/src/SymbolTable.ch` | Symbol table bindings |
| `lang/libs/compiler/src/ASTDiagnoser.ch` | Diagnostics bindings for plugins |
| `lang/libs/compiler/src/BatchAllocator.ch` | Arena allocator for AST nodes |
| `lang/libs/compiler/src/SourceProvider.ch` | Source file provider |
| `lang/libs/lab/src/lab.ch` | `BuildContext`, `Module`, `LabJob` interfaces used by `build.lab` |
| `compiler/cbi/model/CompilerBinder.h` | C++ side — binds CBI functions to plugin symbols |
| `compiler/cbi/model/CBIFunctionType.h` | C++ side — function type definitions for CBI |
| `compiler/cbi/model/ASTBuilder.h` | C++ side — base `ASTBuilder` class (arena allocation) |
| `compiler/cbi/model/Model.h` | C++ side — CBI function typedefs / data model |
| `compiler/cbi/bindings/CBI.cpp` | CBI symbol maps (`*SymMap`) and `prepare_cbi_maps` |

## Plugin Structure

### Basic Plugin Template

Plugin entrypoints are `@no_mangle` `public` functions with the exact C signatures declared in `compiler/cbi/model/Model.h`. The parser/symbol-resolver/codegen call them when the corresponding index was registered.

```chemical
// lang/libs/my_plugin/src/main.ch

// Parse hook: called when the parser encounters `#my_macro`
@no_mangle
public func my_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)
    // consume tokens, parse the macro body, and build an EmbeddedNode
    // (see html_cbi's html_parseMacroNode for a full example)
    return builder.make_embedded_node(
        AccessSpecifier.Internal, std::string_view("my_macro"), data_ptr,
        node_known_type_func, node_child_res_func,
        std::span<*mut ASTNode>(...), std::span<*mut Value>(...), parent, loc)
}

// Symbol resolution hook: link identifiers found inside the macro
@no_mangle
public func my_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    visitor.visitEmbeddedNode(node)
    const resolver = visitor.getSymbolResolver()
    // ... resolve the macro's embedded nodes ...
}

// Replacement hook: return the AST node that finally generates code
@no_mangle
public func my_replacementNode(builder : *mut ASTBuilder, diagnoser : *mut ASTDiagnoser, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut MyRoot
    // ... build and return replacement AST ...
}
```

### Plugin Registration in build.lab

```chemical
// build.lab
public func build(ctx : *mut BuildContext, user_job : *mut LabJob) : *mut Module {

    const cbi_name = std::string_view("my_macro");
    const empty_module = ctx.empty_module(std::string_view(""), std::string_view("my_plugin"));
    if(ctx.contains_cbi(&cbi_name)) {
        return empty_module;
    }
    ctx.set_contains_cbi(&cbi_name)

    var cbi = ctx.build_cbi(&cbi_name);
    ctx.put_job_before(cbi, user_job)

    // build dependencies (std, cstd, compiler bindings, ...)
    var std_module = stdMod.build(ctx, cbi);
    var cstd_module = cstdMod.build(ctx, cbi);
    var compiler_module = compilerMod.build(ctx, cbi);
    const dependencies = [ cstd_module, std_module, compiler_module ]
    var src_path = lab::rel_path_to("src")
    const module = ctx.chemical_dir_module(std::string_view(""), std::string_view("my_plugin"), src_path.to_view(), dependencies);
    ctx.add_module(cbi, module)

    // index plugin functions with their CBIFunctionType
    ctx.index_def_cbi_fn(cbi, std::string_view("my_parseMacroNode"), CBIFunctionType.ParseMacroNode);
    ctx.index_def_cbi_fn(cbi, std::string_view("my_symResNode"), CBIFunctionType.SymResNode);
    ctx.index_def_cbi_fn(cbi, std::string_view("my_replacementNode"), CBIFunctionType.ReplacementNode);

    return empty_module;
}
```

## Compiler API Bindings

The compiler API is in `lang/libs/compiler/src/`. These are Chemical files that expose compiler functionality to plugins.

### Key Classes

| Class | File | Purpose |
|-------|------|---------|
| `ASTBuilder` | `ASTBuilder.ch` | Create AST nodes (functions, structs, statements, values, types) |
| `Lexer` | `Lexer.ch` | Tokenize Chemical source code |
| `Parser` | `Parser.ch` | Parse Chemical source into AST |
| `SymbolResolver` | `SymbolResolver.ch` | Resolve symbols in AST |
| `SymbolTable` | `SymbolTable.ch` | Declare and look up symbols |
| `BatchAllocator` | `BatchAllocator.ch` | Arena allocator for AST nodes |
| `SourceProvider` | `SourceProvider.ch` | Source file access |
| `ASTDiagnoser` | `ASTDiagnoser.ch` | Report errors and warnings |
| `Token` | `Token.ch` | Token structure |
| `Position` | `Position.ch` | Source position |
| `Operation` | `Operation.ch` | Operation types |

### ASTBuilder API

The `ASTBuilder` is the most important class for plugin development:

```chemical
// Create types (loc is a ubigint encoded location, e.g. parser.getEncodedLocation(tok)):
var builder = ASTBuilder()
var intType = builder.get_int_type()                       // *mut IntType
var ptrType = builder.make_ptr_type(intType, false, loc)   // *int
var mutPtrType = builder.make_ptr_type(intType, true, loc) // *mut int

// Create values:
var intVal = builder.make_int_value(42, loc)
var stringVal = builder.make_string_value(&view, loc)
var structVal = builder.make_struct_value(structDecl, loc)

// Create statements:
var varDecl = builder.make_varinit_stmt(false, false, "x", intType, intVal, AccessSpecifier.Internal, parent, loc)
var funcCall = builder.make_function_call_value(parentVal, loc)

// Create functions:
var funcDecl = builder.make_function("my_func", returnType, false, parent, loc)
```

## Built-in Plugins

These are the plugins shipped with the compiler. Each demonstrates different CBI patterns:

| Plugin | Location | Purpose | Key Features |
|--------|----------|---------|--------------|
| `html_cbi` | `lang/libs/html_cbi/` | `#html` macro | Parses HTML/JSX (shared `html_comp` AST) and emits `page.append_html*` calls; resolves `#styled` components (`MountStrategy.Styled`) in `#html` |
| `css_cbi` | `lang/libs/css_cbi/` | `#css`, `#styled` macros | `#css` parses CSS properties into style strings; `#styled` declares scoped, CSS-injecting components usable in `#html` |
| `js_cbi` | `lang/libs/js_cbi/` | `#js` macro | Parses JavaScript with the shared `js_syntax` AST / `js_parser` and inlines it into the JS bundle |
| `universal_cbi` | `lang/libs/universal_cbi/` | `#universal` component | SSR + hydration — generates server function + JS hydration; uses the shared `js_syntax` AST |
| `json_cbi` | `lang/libs/json_cbi/` | `#json(Struct)` macro | Auto-generates `std::Serializer`/`std::Deserializer` impls; uses a marker annotation and top-level node hooks |
| `md_cbi` | `lang/libs/md_cbi/` | Markdown processing | Converts markdown to HTML |

Supporting (non-registered) libraries these CBIs build on: `html_comp/`, `html_parser/`, `html_ide/`, `js_syntax/` (shared token/AST definitions), `js_cbi_lexer/` (compiler-side JS lexer), `js_parser/`, `js_ide/`, `css_parser/`, `css_ide/`, `md_parser/`, `md_ide/`, `universal_parser/`, `universal_ide/`.

### Example: html_cbi Structure

```
lang/libs/html_cbi/
├── build.lab               # CBI build script — creates the job and indexes hooks
└── src/
    ├── main.ch             # Plugin entry — @no_mangle parse/symres/replacement + lexer init
    ├── converter/language/ # HtmlRoot -> Chemical AST conversion
    ├── sym_res/
    │   └── sym_res_root.ch # Symbol resolution helpers
    └── utils/
        └── comptime_utils.ch
```

## CBI Function Types

The `CBIFunctionType` enum (`compiler/cbi/model/CBIFunctionType.h`, mirrored in `lang/libs/lab/src/lab.ch`) defines all hook kinds used in the CBI:

```cpp
enum class CBIFunctionType : int {
    InitializeLexer,
    ParseMacroValue,
    ParseMacroNode,
    ParseMacroTopLevelNode,
    ParseMacroMemberNode,
    SymResDeclareTopLevelNode,
    SymResLinkSignatureNode,
    SymResLinkSignatureValue,
    SymResNode,
    SymResValue,
    ReplacementNodeDeclare,
    ReplacementNode,
    ReplacementValue,
    SemanticTokensPut,
    FoldingRangesPut,
    TransformerMain
};
```

Each function type has a corresponding C typedef (declared in `compiler/cbi/model/Model.h`):

```cpp
// InitializeLexer:
typedef void(*EmbeddedLexerInitializeFn)(Lexer* lexer);

// ParseMacroValue:
typedef Value*(*EmbeddedParseMacroValueFn)(Parser* parser, ASTBuilder* builder);

// ParseMacroNode:
typedef ASTNode*(*EmbeddedParseMacroNodeFn)(Parser* parser, ASTBuilder* builder);

// SymResNode:
typedef void(*EmbeddedNodeSymbolResolveFunc)(SymResLinkBody* visitor, EmbeddedNode* value);

// SymResValue:
typedef bool(*EmbeddedValueSymbolResolveFunc)(SymResLinkBody* visitor, EmbeddedValue* value);

// ReplacementNode:
typedef ASTNode*(*EmbeddedNodeReplacementFunc)(ASTBuilder* builder, ASTDiagnoser* diagnoser, EmbeddedNode* value);

// ReplacementValue:
typedef Value*(*EmbeddedValueReplacementFunc)(ASTBuilder* builder, ASTDiagnoser* diagnoser, EmbeddedValue* value);
```

## The CompilerBinder

`CompilerBinder` is the C++ class that manages CBI function registration:

```cpp
struct CBIFunctionKey {
    chem::string_view key;
    CBIFunctionType type;
};

class CompilerBinder {
    // name -> TCC module state (private; accessible via get_cbi_map())
    util::unordered_string_map<CBIData> data;

public:
    // all indexed functions, keyed by (name, CBIFunctionType)
    std::unordered_map<CBIFunctionKey, void*, CBIFunctionHash> hooks_;

    // interface name -> exported symbol table (SourceProvider, Lexer, ASTBuilder, ...)
    std::unordered_map<chem::string_view, std::span<const std::pair<chem::string_view, void*>>> interface_maps;

    // store a compiled CBI module, guarding against overriding an existing one
    bool store_cbi(std::string name, TCCState* state);

    bool contains_cbi(const std::string_view& name);

    // import a compiler interface's functions into a TCC state
    static void import_compiler_interface(const std::span<const std::pair<chem::string_view, void*>>& interface, TCCState* state);

    // index a plugin function for a given (key, type)
    void registerHook(CBIFunctionType type, const chem::string_view& key, void* function);

    // look up an indexed plugin function
    void* findHook(const chem::string_view& key, CBIFunctionType type) const noexcept;

    // resolve a function by name in a compiled TCC state
    const char* index_function(CBIFunctionIndex& index, TCCState* state);
};
```

## Creating a New Plugin

### Step-by-Step

1. **Create the directory structure** (a CBI package is driven by `build.lab`; a `chemical.mod` is optional):
   ```
   lang/libs/my_plugin/
   ├── build.lab
   └── src/
       └── main.ch
   ```

2. **Write main.ch** — one `@no_mangle public func` per hook you want to expose:
   ```chemical
   @no_mangle
   public func my_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
       // ... parse the macro body and build an EmbeddedNode ...
   }
   ```

3. **Write build.lab** — create the CBI job, add the module, and index the functions:
   ```chemical
   public func build(ctx : *mut BuildContext, user_job : *mut LabJob) : *mut Module {
       const cbi_name = std::string_view("my_macro");
       const empty_module = ctx.empty_module(std::string_view(""), std::string_view("my_plugin"));
       if(ctx.contains_cbi(&cbi_name)) {
           return empty_module;
       }
       ctx.set_contains_cbi(&cbi_name)

       var cbi = ctx.build_cbi(&cbi_name);
       ctx.put_job_before(cbi, user_job);
       // ... build deps + ctx.add_module(cbi, module) ...

       ctx.index_def_cbi_fn(cbi, std::string_view("my_parseMacroNode"), CBIFunctionType.ParseMacroNode);
       return empty_module;
   }
   ```

4. **Make it available to consumers** — a module adds `import my_plugin` to its `chemical.mod`; the compiler resolves `lang/libs/my_plugin/`, finds `build.lab`, and runs its `build` function:
   ```chmod
   module my_app
   source "src"
   import std
   import my_plugin
   ```

5. **Test the plugin**:
   ```bash
   cmake-build-debug/TCCCompiler "my_app/chemical.mod" -o my_app.exe -v -frecompile-plugins
   ```

## Debugging Plugins

### Plugin Debug Mode

```bash
--plugin-mode debug_complete  # Full debug info for plugins
```

### Common Plugin Issues

| Issue | Cause | Debug |
|-------|-------|-------|
| Plugin not found | Module directory has no entry point | Ensure it contains `build.lab` or `chemical.mod` |
| Macro handler not called | Hook not indexed | Verify `ctx.index_cbi_fn` / `index_def_cbi_fn` call |
| CBI function not found | Missing TinyCC symbol | Check `tcc_add_symbol` calls |
| AST crash | AST node created incorrectly | Use BatchAllocator for allocation |
| Symbol resolution error | Wrong symbol table context | Use the correct SymbolTable scope |
| TinyCC compile error | Chemical code has syntax errors | Run plugin through compiler independently |

### Testing Plugins

```bash
# All plugin tests (`test_plugin_exe(..., "all")`):
./scripts/test.sh --tcc --plugins

# A single plugin's tests are selected via a build.lab arg passed to the compiler:
cmake-build-debug/TCCCompiler lang/tests/build.lab --arg-test-html --mode debug_complete
cmake-build-debug/TCCCompiler lang/tests/build.lab --arg-test-css  --mode debug_complete
```

## Related Skills

- **Intrinsics & Compiler Reflection** (`.agents/skills/intrinsics_compiler_reflection/SKILL.md`) — Compiler intrinsics that plugins can use for compile-time evaluation, reflection, and metadata access
- **Build System** (`.agents/skills/build_system/SKILL.md`) — How plugins are compiled, loaded, and integrated into the build pipeline
- **Macro Codegen** (`.agents/skills/macro_code_gen/SKILL.md`) — Examples of how existing plugins (html_cbi, universal_cbi) use the CBI API
- **Compiler API** (`.agents/skills/compiler_api/SKILL.md`) — Compiler API bindings for AST construction, lexing, parsing

## CRITICAL: Enum Sync Rule

**When adding a new enum value to the C++ compiler, you MUST also add the same value to the corresponding Chemical binding file.** Failing to do so causes a SIGSEGV crash in all CBI plugins because the enum values are off by 1 between the C++ side and the TCC-compiled plugin side.

### Affected Enums (must stay in sync)

| C++ Enum (must match) | Chemical Binding File |
|------------------------|----------------------|
| `ASTNodeKind` (in `ast/base/ASTNodeKind.h`) | `lang/libs/compiler/src/ast/base/ASTNodeKind.ch` |
| `TokenType` (in `lexer/TokenType.h`) | `lang/libs/compiler/src/ChemicalTokenType.ch` |

### How It Works

CBI plugins are compiled by TCC from Chemical source. They import the `compiler` library, which contains `.ch` files defining enum values that mirror the C++ enums. The TCC-compiled plugin code uses these Chemical enum values in switch statements and comparisons. If the Chemical enum is missing a value that was added to the C++ enum, every value after the insertion point is shifted by 1, causing wrong branches and crashes.

### Real Bug Example

Adding `InlineAsmStmt` to `ASTNodeKind` in C++ (between `PlacementNewNode` and `EnumDecl`) without adding it to `ASTNodeKind.ch` caused `EnumDecl` to be 31 in C++ but 32 in the plugin. Every `switch(node.getKind())` statement took wrong branches, dereferencing garbage pointers, and SIGSEGV at `ASTNodegetKind`.

Adding `AsmKw` to `TokenType` in C++ without adding it to `ChemicalTokenType.ch` caused `RBrace` to be off by 1, breaking `#html` macro parsing entirely.

### The Fix

When adding a new enum value to a C++ enum that is exposed to CBI:
1. Add the value in the C++ enum at the correct position
2. Add the same value in the corresponding `.ch` file at the same position
3. Verify both files have identical ordering for all values

### Which Direction Is Safe?

Values at the **end** of an enum can be added without breaking existing plugins (they won't reference the new value). Values inserted in the **middle** break all subsequent values and MUST be synced immediately.

## Performance Considerations

1. **TinyCC JIT**: Fast compilation but slower generated code — acceptable for build-time plugins
2. **AST allocation**: Always use `BatchAllocator` from the compiler — arena allocation is fast
3. **Minimize CBI calls**: Each call crosses the Chemical→C++ boundary, which has overhead
4. **Plugin caching**: `--cached-plugins` skips recompilation of unchanged plugins
5. **Compile once**: Plugins are compiled once and reused across all jobs in a build
