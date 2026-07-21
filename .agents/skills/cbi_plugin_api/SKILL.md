---
name: Compiler Plugin API (CBI)
description: Comprehensive guide to the Chemical Compiler Binding Interface (CBI) — how compiler plugins are built, registered, and integrated into the compilation pipeline.
---

# Compiler Plugin API (CBI)

The Chemical Compiler Binding Interface (CBI) allows external Chemical code to hook into the compilation process. Plugins are compiled via TinyCC JIT at build time and can intercept lexing, parsing, symbol resolution, and codegen.

## Architecture

### How Plugins Work

1. **Plugin registration**: A `build.lab` script calls `register_plugin()` to register a plugin
2. **Plugin compilation**: The plugin's Chemical source is compiled to C, then JIT-compiled via TinyCC
3. **Plugin initialization**: The plugin's init function is called, which registers hooks
4. **Hook execution**: During compilation, the compiler calls plugin hooks when it encounters registered macros or needs plugin assistance

### Key Files

| File | Purpose |
|------|---------|
| `lang/libs/compiler/` | Compiler API bindings — the API that plugins call |
| `lang/libs/compiler/src/ASTBuilder.ch` | AST node construction — create any Chemical AST node |
| `lang/libs/compiler/src/Lexer.ch` | Lexer bindings — tokenize Chemical source |
| `lang/libs/compiler/src/Parser.ch` | Parser bindings — parse Chemical source |
| `lang/libs/compiler/src/SymbolResolver.ch` | Symbol resolver bindings |
| `lang/libs/compiler/src/SymbolTable.ch` | Symbol table bindings |
| `lang/libs/compiler/src/ASTDiagnoser.ch` | Diagnostics bindings for plugins |
| `lang/libs/compiler/src/BatchAllocator.ch` | Arena allocator for AST nodes |
| `lang/libs/compiler/src/SourceProvider.ch` | Source file provider |
| `compiler/cbi/model/CompilerBinder.h` | C++ side — binds CBI functions to plugin symbols |
| `compiler/cbi/model/CBIFunctionType.h` | C++ side — function type definitions for CBI |
| `compiler/cbi/model/ASTBuilder.h` | C++ side — CBI AST builder functions |
| `compiler/cbi/model/Model.h` | C++ side — CBI data model |
| `compiler/cbi/bindings/CBI.cpp` | All CBI functions with exact names exposed to plugins |

## Plugin Structure

### Basic Plugin Template

```chemical
// lang/libs/my_plugin/src/main.ch

// Import the compiler API
import "compiler"

// Plugin init function — called when the plugin is loaded
func init(binder : &mut CompilerBinder) {
    // Register macro handlers
    binder.register_macro("my_macro", handle_my_macro)
    
    // Register lex hook (optional)
    binder.register_lex_hook(my_lex_hook)
}

// Macro handler
func handle_my_macro(binder : &mut CompilerBinder, params : *ParamReplacerSequence) : ProcessedMacroResult {
    // Process the macro content
    // Return processed AST nodes or values
    
    var result = ProcessedMacroResult()
    // ... build AST nodes ...
    return result
}

// Optional lex hook
func my_lex_hook(binder : &mut CompilerBinder, token : Token) : Token {
    // Modify or replace tokens
    return token
}
```

### Plugin Registration in build.lab

```chemical
// build.lab
func main(binder : &mut CompilerBinder) {
    // Register a plugin
    binder.register_plugin("my_plugin")
    
    // Create a compilation job
    binder.create_job("my_app", LabJobType::Compilation)
    
    // Add sources
    binder.add_source("my_app", "src/main.ch")
    
    // Add dependencies
    binder.add_dependency("my_app", "std")
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
// Create types:
var builder = ASTBuilder()
var intType = builder.create_type_primitive(PrimitiveType::Int)
var ptrType = builder.create_type_pointer(intType, false)  // *int
var mutPtrType = builder.create_type_pointer(intType, true)  // *mut int

// Create values:
var intVal = builder.create_value_int(42)
var stringVal = builder.create_value_string("hello")
var structVal = builder.create_value_struct(structDecl)

// Create statements:
var varDecl = builder.create_stmt_var_init("x", intType, intVal)
var funcCall = builder.create_value_func_call("printf", args)

// Create functions:
var funcDecl = builder.create_func_decl("my_func", params, returnType, body)
```

## Built-in Plugins

These are the plugins shipped with the compiler. Each demonstrates different CBI patterns:

| Plugin | Location | Purpose | Key Features |
|--------|----------|---------|--------------|
| `html_cbi` | `lang/libs/html_cbi/` | `#html` macro | Parses HTML/JSX, generates page.append_html_view() calls |
| `css_cbi` | `lang/libs/css_cbi/` | `#css` macro | Parses CSS properties, generates style strings |
| `js_cbi` | `lang/libs/js_cbi/` | `#js` macro | Inlines JavaScript code into the JS bundle |
| `universal_cbi` | `lang/libs/universal_cbi/` | `#universal` component | SSR + hydration — generates server function + JS hydration |
| `react_cbi` | `lang/libs/react_cbi/` | `#react` component | React-style JSX → runtime calls |
| `preact_cbi` | `lang/libs/preact_cbi/` | `#preact` component | Preact-style JSX → runtime calls |
| `solid_cbi` | `lang/libs/solid_cbi/` | `#solid` component | SolidJS-style JSX |
| `md_cbi` | `lang/libs/md_cbi/` | Markdown processing | Converts markdown to HTML |

### Example: html_cbi Structure

```
lang/libs/html_cbi/
├── chemical.mod            # Module declaration
├── src/
│   ├── main.ch             # Plugin entry — register macro handler
│   ├── parser/
│   │   ├── HtmlParser.ch   # HTML parser
│   │   └── SymResSupport.ch # Symbol resolution helpers
│   └── codegen/
│       └── HtmlCodegen.ch  # Code generation
```

## CBI Function Types

The `CBIFunctionType` enum defines all function signatures used in the CBI:

```cpp
enum class CBIFunctionType {
    ProcessMacro,
    LexHook,
    ParseHook,
    SymResHook,
    TypeCheckHook,
    CodegenHook,
    Init,
    // ...
};
```

Each function type has a corresponding signature:

```cpp
// ProcessMacro:
using ProcessMacroFunc = ProcessedMacroResult(*)(CompilerBinder*, ParamReplacerSequence*);

// LexHook:
using LexHookFunc = Token(*)(CompilerBinder*, Token);

// Init:
using InitFunc = void(*)(CompilerBinder*);
```

## The CompilerBinder

`CompilerBinder` is the C++ class that manages CBI function registration:

```cpp
class CompilerBinder {
    // Registered plugins
    std::unordered_map<chem::string, PluginInfo> plugins;
    
    // Macro → handler mapping
    std::unordered_map<chem::string, ProcessMacroFunc> macroHandlers;
    
    // Lex hooks
    std::vector<LexHookFunc> lexHooks;
    
    // Register a plugin
    void registerPlugin(const chem::string& name, void* initFunc);
    
    // Register a macro handler
    void registerMacro(const chem::string& name, void* handlerFunc);
    
    // Dispatch a macro
    ProcessedMacroResult dispatchMacro(const chem::string& name, ParamReplacerSequence* params);
};
```

## Creating a New Plugin

### Step-by-Step

1. **Create the directory structure**:
   ```
   lang/libs/my_plugin/
   ├── chemical.mod
   └── src/
       └── main.ch
   ```

2. **Write chemical.mod**:
   ```chmod
   module my_plugin
   
   source "src"
   import "compiler"
   ```

3. **Write main.ch**:
   ```chemical
   func init(binder : &mut CompilerBinder) {
       binder.register_macro("my_macro", handle_my_macro)
   }
   
   func handle_my_macro(binder : &mut CompilerBinder, params : *ParamReplacerSequence) : ProcessedMacroResult {
       var result = ProcessedMacroResult()
       // ... implementation ...
       return result
   }
   ```

4. **Register in build.lab**:
   ```chemical
   func main(binder : &mut CompilerBinder) {
       binder.register_plugin("my_plugin")
   }
   ```

5. **Test the plugin**:
   ```bash
   cmake-build-debug/TCCCompiler "my_app/build.lab" -o my_app.exe -v -frecompile-plugins
   ```

## Debugging Plugins

### Plugin Debug Mode

```bash
--plugin-mode debug_complete  # Full debug info for plugins
```

### Common Plugin Issues

| Issue | Cause | Debug |
|-------|-------|-------|
| Plugin not found | Path to plugin source is wrong | Check chemical.mod path |
| Macro handler not called | Hook not registered | Verify `register_macro` call |
| CBI function not found | Missing TinyCC symbol | Check `tcc_add_symbol` calls |
| AST crash | AST node created incorrectly | Use BatchAllocator for allocation |
| Symbol resolution error | Wrong symbol table context | Use the correct SymbolTable scope |
| TinyCC compile error | Chemical code has syntax errors | Run plugin through compiler independently |

### Testing Plugins

```bash
# Individual plugin tests:
./scripts/test.sh --tcc --plugins --arg-test-html  # Test html_cbi only
./scripts/test.sh --tcc --plugins --arg-test-css   # Test css_cbi only
./scripts/test.sh --tcc --plugins                  # Test all plugins
```

## Related Skills

- **Intrinsics & Compiler Reflection** (`.agents/skills/intrinsics_compiler_reflection/SKILL.md`) — Compiler intrinsics that plugins can use for compile-time evaluation, reflection, and metadata access
- **Build System** (`.agents/skills/build_system/SKILL.md`) — How plugins are compiled, loaded, and integrated into the build pipeline
- **Macro Codegen** (`.agents/skills/macro_code_gen/SKILL.md`) — Examples of how existing plugins (html_cbi, universal_cbi) use the CBI API
- **Compiler API** (`.agents/skills/compiler_api/SKILL.md`) — Compiler API bindings for AST construction, lexing, parsing

## Performance Considerations

1. **TinyCC JIT**: Fast compilation but slower generated code — acceptable for build-time plugins
2. **AST allocation**: Always use `BatchAllocator` from the compiler — arena allocation is fast
3. **Minimize CBI calls**: Each call crosses the Chemical→C++ boundary, which has overhead
4. **Plugin caching**: `--cached-plugins` skips recompilation of unchanged plugins
5. **Compile once**: Plugins are compiled once and reused across all jobs in a build
