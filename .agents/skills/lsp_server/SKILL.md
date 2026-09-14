---
name: LSP Server
description: Comprehensive deep-dive into the Chemical language server (`ChemicalLsp`) — the `server/` subsystem, how it is built on `lib/lsp-framework`, its document/state model (`WorkspaceManager`, `ModuleData`), how it drives `SymbolResolver` for incremental analysis, every implemented LSP capability and where it lives, `chemical.mod` handling, diagnostics mapping, CBI analyzer hooks, and how to build and test the server. Load when working on the language server, editor features, semantic tokens, folding ranges, hover, completion, go-to-definition, signature help, inlay hints, formatting, `server/` internals, `LSPHooks`, or the `chemical/*` IDE libraries.
---

# LSP Server (`ChemicalLsp`)

The Chemical language server is a standalone C++ executable, `ChemicalLsp`, that speaks
the Language Server Protocol over **stdio** or a **TCP socket**. It is editor-driven and
incremental: it keeps a long-lived workspace of parsed and symbol-resolved modules and
answers IDE requests (diagnostics, completion, hover, navigation, semantic tokens, …)
against the **in-editor buffer**, not the on-disk file.

It is *not* the batch compiler. `Compiler` and `TCCCompiler` are intentionally
short-lived, do full end-to-end codegen, and share `compiler_main()`. `ChemicalLsp` has
its own `main()` (`core/targets/LSPMain.cpp:359`), never emits object code, and only
re-runs the **front-end** of the compiler (lex → parse → symbol resolution). It still
links TinyCC and `lib/lsp-framework`, and it shells out to `compiler_main` for the `cc`
subcommand and to a child `ChemicalLsp` process to evaluate `build.lab`/`chemical.mod`.

Related: **CLI Entrypoint and Targets** (`.agents/skills/cli_entrypoint/SKILL.md`),
**Symbol Resolution** (`.agents/skills/symres/SKILL.md`),
**Diagnostics** (`.agents/skills/diagnostics/SKILL.md`),
**Compiler Bindings** (`.agents/skills/compiler_bindings/SKILL.md`),
**Build System** (`.agents/skills/build_system/SKILL.md`).

## 1. Key Files and Directories

| Path | Purpose |
|------|---------|
| `core/targets/LSPMain.cpp` | `ChemicalLsp` `main()`, capability advertisement, request/notification handler registration, stdio & socket transports, `--build-lab` child mode, DEBUG `--run-tests` |
| `server/WorkspaceManager.h/.cpp` | Per-client operations manager: caches, document state, request dispatch, build-context bootstrap |
| `server/LspSemanticTokens.cpp` | The bulk of the state machine: parsing modules, symbol resolution, caching/dirty tracking, semantic tokens, diagnostics publishing, file index (new/deleted) |
| `server/Importer.cpp` | `get_lexed()` — lex from overridden in-memory source or disk |
| `server/mod_file/Importer.cpp` | `process_dot_mod_file()` — parse and diagnose `chemical.mod` |
| `server/analyzers/` | All IDE feature analyzers (completion, hover, goto-def, symbols, folding, formatting, signature help, inlay hints, semantic tokens, caret utils) |
| `server/build/` | Out-of-process build evaluation: `ChildProcessBuild`, `ContextSerialization`, `ipc_process`, `Build.cpp` (`compile_lab`) |
| `server/diagnostics/` | `add_diagnostics()` — compiler `Diag` → `lsp::Diagnostic` |
| `server/model/` | Data model: `ModuleData`, `LexResult`, `ASTResult`, `AnonymousFileData`, `ClientKind`, `SemanticTokenScopes` |
| `server/cbi/hooks.h` | Typedefs for embedded semantic-token / folding-range hooks |
| `server/utils/` | `LRUCache`, `AnalyzerUtils` (`get_token_at_position`), `lspfwd.h`, dead `PrintUtils` |
| `server/tests/LspTests.cpp` | DEBUG-only formatter test harness (`run_lsp_tests`) |
| `compiler/cbi/bindings/lsp/LSPHooks.h/.cpp` | C ABI shims exposing analyzers to CBI plugins |
| `compiler/cbi/bindings/CBI.cpp` | Registers the `LSPAnalyzers` compiler interface (LSP build only) |
| `lib/lsp-framework/` | Vendored C++20 LSP implementation (types/messages generated from the official meta model) |

## 2. Architecture

### 2.1 Foundation: `lib/lsp-framework`

The server is built on the vendored `lib/lsp-framework` submodule
(`CMakeLists.txt:718-720`). That library generates `lsp/types.h` and `lsp/messages.h`
from the official LSP meta model and provides `lsp::Connection`, `lsp::MessageHandler`,
`lsp::io::standardIO()`, and `lsp::io::SocketListener` (see `lib/lsp-framework/README.md`).
All request/notification params and results are typed C++ structs; handlers are
registered with `handler.add<lsp::requests::X>(...)`.

The server target adds `${CMAKE_SOURCE_DIR}/lib/lsp-framework` and the generated-files
dir to its include path (`CMakeLists.txt:852-856`) and links `lsp` + `${LIBTCC_LIB}`
(`CMakeLists.txt:1016`). It is compiled with `LSP_BUILD` (`CMakeLists.txt:768`), which
enables LSP-only code paths throughout the compiler (e.g. `compiler/lab/TargetData.h:81`
sets `target.lsp = true`, surfaced to build scripts as `def.lsp`).

### 2.2 Entry point and transports

`main()` (`core/targets/LSPMain.cpp:359`) registers these CLI options
(`LSPMain.cpp:367-381`): `--resources`/`--res`, `--port`, `--version`/`-v`,
`--build-lab`, `cc`, `--shmName`, `--evtChildDone`, `--evtParentAck`, `--stdio`,
`--client`, and (DEBUG) `--run-tests`.

Three roles:

1. **stdio server** (`--stdio` → `run_stdio_session`, `LSPMain.cpp:325`). The default
   when the option is supplied.
2. **socket server** (default if neither `--stdio` nor `--build-lab`): listens on
   `--port` (default `5007`) via `lsp::io::SocketListener`; each accepted connection is
   served by a detached thread running `run_session` (`LSPMain.cpp:271`, acceptor loop
   at `LSPMain.cpp:485-498`). One process can serve multiple clients, each with its own
   `WorkspaceManager`.
3. **build-lab child** (`--build-lab <file>` + IPC names, `LSPMain.cpp:429-459`): calls
   `compile_lab()` to evaluate a `build.lab`/`chemical.mod` and report the resulting
   module/job graph back to the parent over shared memory.

Both session functions create an `lsp::Connection`, a `lsp::MessageHandler`, and a
`WorkspaceManager(exePath, handler)`, then call `registerDefaultHandlers()` and loop on
`handler.processIncomingMessages()`.

`--client <zed|vscode|intellij>` overrides client detection (`LSPMain.cpp:397-408`);
otherwise `WorkspaceManager::initialize` sniffs `params.clientInfo.name`
(`WorkspaceManager.cpp:213-222`). `ClientKind` (`server/model/ClientKind.h`) currently
only affects the advertised token-type legend.

### 2.3 `registerDefaultHandlers` — the LSP surface

`registerDefaultHandlers` (`LSPMain.cpp:49-268`) is the single place where every
capability is wired to a `WorkspaceManager` method:

| LSP message | Handler location | Manager call |
|-------------|------------------|--------------|
| `initialize` | `LSPMain.cpp:54` | `manager.initialize(params)` + capability result |
| `initialized` | `LSPMain.cpp:124` | `manager.register_watched_files_capability()` |
| `textDocument/semanticTokens/full` | `LSPMain.cpp:131` | `get_semantic_tokens_full` |
| `textDocument/foldingRange` | `LSPMain.cpp:146` | `get_folding_range` |
| `textDocument/documentSymbol` | `LSPMain.cpp:154` | `get_symbols` |
| `textDocument/hover` | `LSPMain.cpp:162` | `get_hover` |
| `textDocument/definition` | `LSPMain.cpp:171` | `get_definition` |
| `textDocument/completion` | `LSPMain.cpp:180` | `get_completion` |
| `textDocument/signatureHelp` | `LSPMain.cpp:189` | `get_signature_help` |
| `textDocument/inlayHint` | `LSPMain.cpp:198` | `get_hints` |
| `textDocument/formatting` | `LSPMain.cpp:209` | `get_formatting` |
| `textDocument/didOpen` | `LSPMain.cpp:217` | `OnOpenedFile` |
| `textDocument/didChange` | `LSPMain.cpp:225` | `onChangedContents` |
| `textDocument/didSave` | `LSPMain.cpp:233` | `onSave` |
| `workspace/didChangeWatchedFiles` | `LSPMain.cpp:241` | `index_new_file` / `de_index_deleted_file` |
| `shutdown` | `LSPMain.cpp:261` | sets the session shutdown flag |

Advertised capabilities (`LSPMain.cpp:103-115`): incremental text sync, completion
(trigger chars `.` `::`), hover, signature help (trigger `,`), definition, document
symbol, document formatting, folding range, full semantic tokens, and inlay hints.
`positionEncoding` is **not** set (the framework default applies).

### 2.4 `WorkspaceManager` — the state model

`WorkspaceManager` (`server/WorkspaceManager.h:67`) is created once per client session
and owns all caches and state. It is documented as outliving an individual session but
dying on client disconnect. Key members:

| Member | Type | Purpose |
|--------|------|---------|
| `overriddenSources` | `unordered_map<string,string>` | In-memory editor buffers not yet saved to disk |
| `incremental_change_mutex` | `mutex` | Serializes `didChange` edits |
| `process_file_mutex` | `mutex` | Only one file is processed (parsed + symresolved) at a time |
| `tokenCache` | `LRUCache<string, shared_ptr<LexResult>>` (cap 10) | Lexed tokens per file |
| `filesIndex` | `unordered_map<string_view, ModuleData*>` | Fast file → module lookup |
| `moduleData` | `unordered_map<LabModule*, unique_ptr<ModuleData>>` | Per-module AST/state cache |
| `dirtyModules` | `unordered_set<ModuleData*>` | Modules needing re-symbol-resolution |
| `modFileData` | `LRUCache<string, shared_ptr<ModuleFileDataUnit>>` (cap 10) | Parsed `chemical.mod` units |
| `anonFilesData` | `LRUCache<string, shared_ptr<AnonymousFileData>>` (cap 10) | Files not belonging to any module |
| `loc_man` | `LocationManager` | Single location table; locations must be disposed when cached files change |
| `controller` | `AnnotationController` | Annotation handling |
| `global_allocator` | `ASTAllocator` | Session-lifetime arena |
| `modStorage` / `typeBuilder` / `pathHandler` / `coreNodes` / `implsIndex` / `instContainer` / `binder` | compiler services | Shared resolver state |
| `context_information` | `BuildContextInformation` | Serialized build context from the child build |
| `pool` | `ctpl::thread_pool` | Parsing/symres tasks (hardware concurrency threads) |
| `main_job`, `global_container` | — | Main `LabJob*`; lazily created `GlobalContainer*` |

`ModuleData` (`server/model/ModuleData.h:47`) holds a module-level `ASTAllocator`, a
`ModuleScope`, a map of `CachedASTUnit*` (`ModuleData.h:15`) per file, a `dirtyFiles`
set, and `dependencies`. `completely_symbol_resolved()` is
`symbol_resolved_once && dirtyFiles.empty()` (`ModuleData.h:123`).

`LexResult` (`server/model/LexResult.h:12`) holds the token vector plus lexer `diags`
and either a `FileInputSource` (disk) or `overridden_source` (editor). Tokens point
into that source string, so it must not change after lexing. `ASTResult`
(`server/model/ASTResult.h:13`) wraps an `ASTUnit` plus its allocator and diagnostics.

### 2.5 Initialization sequence

`initialize()` (`WorkspaceManager.cpp:210`) records the project path from
`rootUri`/`rootPath`, then calls `build_context_from_build_lab()` off the thread pool.
That function (`WorkspaceManager.cpp:176`) looks for `chemical.mod` then `build.lab`,
launches a child build (`launch_child_build`), and on success calls `post_build_lab()`
(`WorkspaceManager.cpp:97`):

1. Set `main_job` to the first job in the serialized context.
2. `index_module_files()` (`WorkspaceManager.cpp:79`) — for every module in
   `modStorage`, call `ASTProcessor::determine_module_files(...)` and index each file
   path in `filesIndex` to its `ModuleData`.
3. `compile_cbi(job)` (`WorkspaceManager.cpp:118`) for every `LabJobType::CBI` job —
   compiles the plugin **in-process** via `LabBuildCompiler::do_job`, leaving the TCC
   state in the binder so macro/analyzer hooks are callable.

#### Out-of-process evaluation of `build.lab`

Evaluating a build script requires running arbitrary Chemical code and leaving a `TCCState`
behind, which the parent cannot reuse directly. So the server evaluates it in a child
process and serializes the *resulting data*:

- `launch_child_build` (`server/build/ChildProcessBuild.cpp:389`) either compiles
  in-process under `DEBUG_CHILD_BUILD` (DEBUG only, for debugger friendliness) or
  performs IPC: `get_child_build_payload` (`ChildProcessBuild.cpp:208` POSIX,
  `:29` Windows) spawns `<lsp> --build-lab <file> --shmName … --evtChildDone …
  --evtParentAck …`, waits up to 10 s on a semaphore/event, and reads a shared-memory
  payload.
- The child runs `compile_lab` (`server/build/Build.cpp:13`), which builds the lab to a
  TCC state via `compiler.built_lab_file(...)`, invokes `chemical_lab_build`, and
  serializes the module graph + jobs with `labBuildContext_toJsonStr`
  (`server/build/ContextSerialization.cpp`). `child_create_and_write_shm`
  (`server/build/ipc_process.cpp:365`) writes it to shared memory.
- The parent deserializes with `labBuildContext_fromJson` into
  `context_information` (`server/build/ContextSerialization.h:18`), reconstructing
  `ModuleStorage` modules, dependencies, symbol-info pools, and `LabJob`s.

### 2.6 Document lifecycle

- **open** — `OnOpenedFile` (`WorkspaceManager.cpp:411`) → `process_any_file_on_open`
  (`LspSemanticTokens.cpp:1012`): `.mod` → `process_dot_mod_file`; `.lab` → parse;
  otherwise skip if already cached (tokens present), else `process_file`.
- **change** — `onChangedContents` (`WorkspaceManager.cpp:471`) under
  `incremental_change_mutex`. It loads the current source (overridden or disk), applies
  each `TextDocumentContentChangeEvent`. A whole-document change is fast-pathed
  (`:507-516`); range changes are applied by `positionToOffset`/`applyChange`
  (`WorkspaceManager.cpp:416,451`). The new text is stored in `overriddenSources` and
  `process_any_file(path, true, false)` re-lexes, re-parses, re-symbol-resolves, and
  republishes diagnostics.
- **save** — `onSave` (`WorkspaceManager.cpp:565`): if the saved file is
  `chemical.mod`/`*.lab`, the entire module storage, module data, token cache, file
  index, instantiation container, core nodes, and impls index are cleared and the
  build context is rebuilt from scratch.
- **close** — `onClosedFile` (`WorkspaceManager.cpp:588`) drops the overridden source.
  (`textDocument/didClose` is not currently registered; the method exists for future
  use.)

### 2.7 How this differs from the batch compiler

| Aspect | Batch (`Compiler`/`TCCCompiler`) | LSP (`ChemicalLsp`) |
|--------|----------------------------------|---------------------|
| Lifetime | one invocation | long-lived, editor session |
| Input | files on disk | editor buffers (`overriddenSources`) |
| Pipeline | lex → symres → typecheck → 2c → codegen → link | lex → parse → symbol resolution only |
| Type verification | yes (`TypeVerifier`) | **not run** |
| Generic instantiation | full finalization | registration/signature only, per module |
| State | fresh per run | cached `ModuleData`/`LexResult`, dirty tracking |
| Concurrency | per-file passes, allocators cleared per pass | thread pool for parsing, one file processed at a time |

## 3. How Analysis Reuses Compiler Passes

### 3.1 `process_file` — the heart of the server

`WorkspaceManager::process_file` (`server/LspSemanticTokens.cpp:629`) is the single path
that makes a file "ready" for analysis. Steps:

1. **Lex** the file with comments (`get_lexed(path, /*keep_comments=*/true)`). On lex
   errors it publishes lexer diagnostics, stores tokens in `tokenCache`, and **returns
   early** — no parsing (`LspSemanticTokens.cpp:651-667`). This is the primary handling
   of a broken/erroneous buffer.
2. Copy tokens (excluding comment tokens) into a `copied_tokens` vector while recording
   original indices. Parsing mutates token `linked` pointers; after symbol resolution the
   links are copied back to the original tokens by index
   (`LspSemanticTokens.cpp:973-985`). This keep-links-out-of-comments arrangement is why
   semantic tokens can later map tokens to AST nodes.
3. Build a `GlobalInterpretScope` and a `SymbolResolver` with a 10 KB resolver
   allocator, then `bind_or_create_container` (`LspSemanticTokens.cpp:515`) creates or
   rebinds the session-wide `GlobalContainer`.
4. Look up `ModuleData` by file path (`getModuleData`). If none, the file is
   **anonymous** and handled separately (see §3.4).
5. If it belongs to a module: `parseModuleWithDepsWait` parses the module and all
   dependencies once (`:735`), then `sym_res_mod_deps_seq` symbol-resolves dependencies
   (`:743`).
6. Decide whether the **current module** needs re-resolution
   (`sym_res_curr_mod`): true if a dependency was re-resolved, or a *different* file in
   the module is dirty (`:750-772`).
7. Re-declare/link the module's other files if needed, always declaring dependencies
   first. Then `resolver.declare_and_link_file(...)` on the requested file
   (`:914`).
8. Copy linked AST nodes back onto the original tokens, cache tokens in `tokenCache`,
   and publish diagnostics built from lexer + parser + resolver diagnostics
   (`:990-997`).

### 3.2 The six front-end passes

Full module symbol resolution is performed in `sym_res_mod_sig`
(`LspSemanticTokens.cpp:322`) using `SymbolResolver` directly (not `ASTProcessor`):

```
module_scope_start()
  → removeInstantiationsFor(file) + clear module allocator
  → setASTAllocator(module allocator)
  → declareDependencies()                     // recursive
  → resolver.tld_declare_file(...)            // Pass 1: top-level declarations
  → resolver.link_signature_file(...)         // Pass 2: signatures
  → resolver.generic_instantiation_file(...)  // Pass 3: generic registration
  → resolver.after_link_signature_file(...)   // Pass 4: after-link-signature
  → module_scope_end()
  → unmake_module_dirty(modData)
```

This mirrors the batch compiler's serial `ASTProcessor` passes (see
`.agents/skills/build_system/SKILL.md`), but is scoped to the modules that changed
and **does not** run body linking for files other than the requested one in the
"not dirty" case — that path only *declares* other module files
(`SymbolResolverDeclarer`, `LspSemanticTokens.cpp:850-858`). The requested file always
gets full `declare_and_link_file` (`:914`), which runs declaration + signature + body
linking for that file.

### 3.3 Incomplete / erroneous buffers

The server phases the pipeline so a broken buffer still yields useful output:

- **Lex error** → publish lexer diagnostics, cache tokens, stop (`:651-667`). Semantic
  tokens and folding ranges still work because they only need tokens.
- **Parse/symres errors** → `parse_file` returns a bool and moves `parser.diagnostics`
  into `parse_diagnostics` (`:37-78`); `resolver` collects its own diagnostics. All are
  appended and published (`:990-997`).
- **Anonymous / no module** → `process_file` still parses and links the file into an
  `AnonymousFileData` with a throwaway module scope (`:928-966`), so a single open file
  outside a project still gets highlighting/diagnostics.
- **Dirty tracking** prevents stale results: `make_module_dirty` / `unmake_module_dirty`
  (`WorkspaceManager.h:350,358`) use a single-file dirty model. `should_process_file`
  (`LspSemanticTokens.cpp:550`) short-circuits resolution when nothing relevant changed,
  and checks `exists_in_deps` (`:538`) to decide whether a dirty dependency forces
  reprocessing.

### 3.4 Caching and instantiation cleanup

- `tokenCache` is an LRU (capacity 10) of `LexResult` per absolute path
  (`WorkspaceManager.h:116`).
- `parseFile` (`LspSemanticTokens.cpp:106`) allocates a 10 KB `CachedASTUnit` per file
  and stores it in `modData->cachedUnits`/`fileUnits`.
- Before clearing a module's allocator, all generic instantiations keyed by file id and
  all declaration-keyed instantiations are removed
  (`instContainer.removeInstantiationsFor`, `removeDeclInstantiations`,
  `LspSemanticTokens.cpp:890-900`), preventing dangling instantiation records after the
  arena is reset.
- `modData->allocator.clear()` happens only when the module is re-resolved
  (`:788`), not on every request.

## 4. Feature Reference

Every analyzer takes decoded `SourceLocation`s via `LocationManager` and either the
cached `ASTUnit` (symbol-resolved features) or the token list (lexical features).
`get_token_at_position` lives in `server/utils/AnalyzerUtils.cpp:10`.

### 4.1 Diagnostics (`textDocument/publishDiagnostics`)

- Compiler `Diag` → `lsp::Diagnostic` in `add_diagnostics`
  (`server/diagnostics/DiagnosticUtils.cpp:5`): maps `diag.range` to `lsp::Range`,
  copies `message`, and casts `diag.severity` directly to `lsp::DiagnosticSeverity`
  (compiler severities are `Error`/`Warning`/`Information`/`Hint`; see the Diagnostics
  skill).
- Published synchronously or asynchronously via `publish_diagnostics`
  (`LspSemanticTokens.cpp:1266`), which cancels any in-flight async publish using
  `publish_diagnostics_cancel_flag` (`WorkspaceManager.h:222`) and an
  `std::async` task.
- Build failures are **not** published as document diagnostics — `report_build_failure`
  (`WorkspaceManager.cpp:596`) sends a custom `chemical/buildStatus` notification with
  `{ success, uri, message }` so it does not clobber fine-grained parse/import
  diagnostics on the same `chemical.mod`/`build.lab` URI.
- `relatedInformation`, `tags`, and fix-its are **not** currently propagated —
  `add_diagnostics` only copies range/message/severity.

### 4.2 Semantic tokens (`textDocument/semanticTokens/full`)

`get_semantic_tokens_full` (`LspSemanticTokens.cpp:1037`) ensures the file is processed,
then `get_semantic_tokens` (`:1288`) runs `SemanticTokensAnalyzer`
(`server/analyzers/SemanticTokensAnalyzer.cpp`). It walks tokens and emits the
delta-encoded 5-int tuples LSP expects. `put_auto` (`:115`) maps keyword token types to
`SemanticTokenScopes`, and `put_node_token` (`:56`) maps a token's linked AST node kind
to a scope (functions, structs, enums, variables, params, namespaces, …). Strings,
chars, comments (including multiline splitting via `putMultilineToken`, `:386`), and
numbers are handled explicitly. `#macro` tokens dispatch to a CBI hook (§7).
Token type ordering is fixed by `getTokenTypes` in `LSPMain.cpp:37` and mirrored by
`SemanticTokenScopes` (`server/model/SemanticTokenScopes.h`).

### 4.3 Completion (`textDocument/completion`)

`get_completion` (`WorkspaceManager.cpp:287`) → `CompletionItemAnalyzer`
(`server/analyzers/CompletionItemAnalyzer.cpp`). `analyze` (`:414`) reverses the current
file's top-level nodes, descends into the scope containing the caret, and emits
completions for top-level symbols of the current file, the module, and its direct
dependencies. Completion items carry markdown docs and small details built by
`markdown_documentation` / `small_detail_of`
(`server/analyzers/Documentation.h`, implemented in `HoverAnalyzer.cpp:49,124`).
Note many member-level completions are still `//TODO` stubs (access-chain completion
before the caret is detected via `chain_before_caret` but not yet resolved,
`CompletionItemAnalyzer.cpp:419-427`).

### 4.4 Hover (`textDocument/hover`)

`get_hover` (`WorkspaceManager.cpp:399`) → `HoverAnalyzer::markdown_hover`
(`server/analyzers/HoverAnalyzer.cpp:261`). It finds the token at the caret, follows
`token->linked` (either directly an `ASTNode`, or via `get_ref_linked_node()`), and
renders markdown with `markdown_documentation` (`HoverAnalyzer.cpp:124`): "Defined in"
relative path plus a fenced code signature for functions, structs, interfaces, enums,
variables, params, and typealiases.

### 4.5 Go-to-definition (`textDocument/definition`)

`get_definition` (`WorkspaceManager.cpp:373`) → `GotoDefAnalyzer::analyze`
(`server/analyzers/GotoDefAnalyzer.cpp:22`). It reads the token at the caret, follows its
linked node, decodes the definition's `SourceLocation`, and returns one `DefinitionLink`
with the defining file URI and range. No multi-file related bindings.

### 4.6 Document symbols (`textDocument/documentSymbol`)

`get_symbols` (`WorkspaceManager.cpp:385`) → `DocumentSymbolsAnalyzer`
(`server/analyzers/DocumentSymbolsAnalyzer.cpp`). Visitors are `VisitFunctionDecl`
(`:69`), `VisitStructDecl`, `VisitUnionDecl`, `VisitVariantDecl`, `VisitInterfaceDecl`,
`VisitTypealiasStmt`, `VisitEnumDecl`, `VisitVarInitStmt` (`:75-101`). Nested symbols
and detailed signatures are TODO (`:14-16`).

### 4.7 Folding ranges (`textDocument/foldingRange`)

`get_folding_range` (`WorkspaceManager.cpp:264`) → `FoldingRangeAnalyzer`
(`server/analyzers/FoldingRangeAnalyzer.cpp`). `analyze` (`:58`) scans tokens tracking a
brace stack (`LBrace`/`RBrace`, `:31-40`) and emits region ranges; `#macro` tokens
dispatch to a CBI hook (`:41-50`). `folding_range` (`:17`) tags comment vs region.

### 4.8 Formatting (`textDocument/formatting`)

`get_formatting` (`WorkspaceManager.cpp:332`) lexes with comments and reads the source
(overridden or disk), then `FormatterAnalyzer::format`
(`server/analyzers/FormatterAnalyzer.cpp:6`) produces a whole-file `TextEdit`. It
regenerates indentation, spacing, and newlines from tokens, preserving comments,
handling `else`/`else if`, arrays, annotations, and collapsing runs of blank lines to at
most one. This is the only feature tested by `server/tests/LspTests.cpp`.

### 4.9 Signature help (`textDocument/signatureHelp`)

`get_signature_help` (`WorkspaceManager.cpp:353`) → `SignatureHelpAnalyzer::analyze`
(`server/analyzers/SignatureHelpAnalyzer.cpp:50`). It scans left from the caret to find
the unmatched `(`'s preceding identifier (`:74-95`), resolves the identifier's linked
node → `known_type()` → `FunctionType`, and builds one `SignatureInformation` with
parameter labels and an active parameter based on the caret line
(`:127-154`).

### 4.10 Inlay hints (`textDocument/inlayHint`)

`get_hints` (`WorkspaceManager.cpp:317`) → `inlay_hint_analyze`
(`server/analyzers/InlayHintAnalyzer.cpp:17`, API in `InlayHintAnalyzerApi.h`). The
`RecursiveVisitor` skips nodes outside the requested `Range` (`should_compute`, `:27`)
and emits:
- function-call argument-name hints (`VisitFunctionCall`, `:36`, using
  `FunctionType::func_param_for_arg_at`), and
- inferred-type hints for `var` declarations without an explicit type
  (`VisitVarInitStmt`, `:78`).

### 4.11 Watched files (create/delete)

`register_watched_files_capability` (`LspSemanticTokens.cpp:1214`) registers a watcher
for `**/*.{ch,mod}` with create+delete watch kinds. `index_new_file`
(`:1094`) finds the owning module via `find_module_parent_of` (`:1082`), appends the
file to `mod->direct_files`, parses it, and adds it to `filesIndex`.
`de_index_deleted_file` (`:1147`) removes it from `cachedUnits`, `fileUnits`,
`filesIndex`, and `direct_files`. Uses `weakly_canonical` since deleted paths throw in
`canonical`.

### 4.12 Not implemented

There are **no** handlers for `textDocument/references`, `rename`/`prepareRename`,
`codeAction`, `documentHighlight`, `workspace/symbol`, `completionItem/resolve`,
`documentLink` (the analyzer exists but returns an empty vector —
`server/analyzers/DocumentLinksAnalyzer.cpp:7`), or `executeCommand`. Do not document or
call these as if they exist.

## 5. `chemical.mod` Handling in the Editor

`.mod` files do **not** go through the module `SymbolResolver` pipeline as Chemical
source; they are parsed for structure and diagnosed for missing paths/imports.

`WorkspaceManager::process_any_file` (`LspSemanticTokens.cpp:1001`) routes any path
ending in `chemical.mod` to `process_dot_mod_file`
(`server/mod_file/Importer.cpp:81`):

1. Lex with comments; store tokens in `tokenCache`.
2. Copy non-comment tokens while recording original indices.
3. Get or create a `ModuleFileDataUnit` (LRU), clearing it for reuse.
4. `BasicParser::parseModuleFile(unit->allocator, unit->modFileData)` parses the `.mod`
   into `ModuleFileData` (`compiler/processor/ModuleFileData.h:79`) — sources list,
   link libs, ship files, include dirs, C files, compiler interfaces, options.
5. Copy AST links back to the original tokens (same token/index mapping as Chemical
   files).
6. `diagnoseModuleFileDataUnit` (`server/mod_file/Importer.cpp:31`):
   - every `source "<path>"` is resolved relative to the `.mod` and must exist;
   - every leading `import` statement is checked:
     - native imports (`std`, `cstd`, …) via `pathHandler.resolve_native_lib` and
       `containsModOrLab` (`:14`), which requires a `chemical.mod` or `build.lab` in the
       resolved library directory;
     - local imports via `resolve_sibling` + `containsModOrLab`.
7. Diagnostics are converted and published for the `.mod` URI
   (`server/mod_file/Importer.cpp:150-153`).

Because the project's build context is loaded from `chemical.mod`, saving it triggers a
full context rebuild (`onSave`, §2.6). `.lab` files are, for now, handled by the normal
`process_file` path (`LspSemanticTokens.cpp:1004-1006`).

## 6. Diagnostics Mapping Detail

- Source: parser (`parser.diagnostics`), lexer (`LexResult::diags`), and
  `SymbolResolver` diagnostics (`resolver.diagnostics`), accumulated in
  `process_file` (`LspSemanticTokens.cpp:990-997`).
- Conversion: `add_diagnostics` (`server/diagnostics/DiagnosticUtils.cpp:5`) assumes
  `diag.severity.has_value()` — `Diag::severity` is an `std::optional<DiagSeverity>` and
  is cast straight to `lsp::DiagnosticSeverity`. Ranges come pre-computed on `Diag`
  from `LocationManager`-backed positions.
- Publishing: `publish_diagnostics` (`LspSemanticTokens.cpp:1266`) uses
  `lsp::notifications::TextDocument_PublishDiagnostics` (which replaces all diagnostics
  for a URI), so concurrent publishes are serialized by `publish_diagnostics_mutex` and
  the previous async task is cancelled.
- Multi-file/related info: not emitted today. Definition links and hover do carry a
  cross-file URI via `LocationManager::getPathForFileId`, but `Diag`'s
  `relatedInformation`/`tags`/`fixits_` are dropped by the converter.

## 7. CBI Integration in the Server

Compiler plugins can highlight and fold their embedded languages (`#html`, `#css`,
`#js`, `#md`, `#universal`) inside the editor.

### 7.1 Analyzer hooks

`server/cbi/hooks.h` defines two function-pointer typedefs:
`EmbeddedSemanticTokensPut` and `EmbeddedFoldingRangesPut`. When
`SemanticTokensAnalyzer::put_auto` sees a `TokenType::HashMacro`
(`SemanticTokensAnalyzer.cpp:318-331`) it strips the leading `#`, looks up the hook by
name + `CBIFunctionType::SemanticTokensPut` in the binder, and calls it
(`((EmbeddedSemanticTokensPut) hook)(this, &t, end_token)`). `FoldingRangeAnalyzer` does
the same for `CBIFunctionType::FoldingRangesPut`
(`FoldingRangeAnalyzer.cpp:41-50`). The hook returns the next token to continue from.

### 7.2 C ABI shims

`compiler/cbi/bindings/lsp/LSPHooks.h/.cpp` implement the `extern "C"` symbols the
TCC-compiled plugin calls: `SemanticTokensAnalyzerputAuto`, `…put`, `…putToken`,
`FoldingRangeAnalyzerput`, `…stackPush`, `…stackEmpty`, `…stackPop`. They are thin
wrappers over the C++ analyzer methods (e.g. `LSPHooks.cpp:8-57`).

### 7.3 Interface registration

`compiler/cbi/bindings/CBI.cpp:468-478` defines `LSPAnalyzersMap` (prefix `ide_`) and
registers it as the `LSPAnalyzers` compiler interface **only under `LSP_BUILD`**
(`CBI.cpp:493-495`). On the Chemical side,
`lang/libs/ide/src/SemanticTokensAnalyzer.ch` and `FoldingRangeAnalyzer.ch` declare the
matching `@compiler.interface`s.

### 7.4 Plugin wiring

A plugin's `build.lab` conditionally indexes its hook functions when building for the
LSP, using the `def.lsp` target flag:

```chemical
comptime if(def.lsp) {
    ctx.index_def_cbi_fn(cbi, std::string_view("md_semanticTokensPut"), CBIFunctionType.SemanticTokensPut);
    ctx.index_def_cbi_fn(cbi, std::string_view("md_foldingRangesPut"), CBIFunctionType.FoldingRangesPut);
}
```

The implementations live in `lang/libs/<x>_ide/src/semantic_tokens.ch` and
`folding_ranges.ch` as `@no_mangle` functions
(e.g. `md_semanticTokensPut` in `lang/libs/md_ide/src/semantic_tokens.ch:68`). They call
`analyzer.putToken(...)`/`analyzer.put(...)`/`analyzer.stackPush/Pop`. Hook lookup and
storage are in `CompilerBinder::findHook` / `registerHook`
(`compiler/cbi/model/CompilerBinder.h:102,110`); the enum values live in
`compiler/cbi/model/CBIFunctionType.h:33,35`. Note the **enum-sync rule**: adding a CBI
enum value requires updating `lang/libs/compiler/src/...` mirrors.

## 8. Build and Test

### 8.1 Build

```bash
./scripts/build.sh --lsp              # builds ChemicalLsp
./scripts/build.sh --all              # builds TCCCompiler, Compiler, ChemicalLsp
```

`scripts/build.sh:34` maps `--lsp`/`--ChemicalLsp` to the `ChemicalLsp` CMake target.
Any change under `server/` or `compiler/cbi/bindings/lsp/` requires rebuilding this
target. `ChemicalLsp` is intentionally compiled with RTTI/exceptions enabled (it links
against `lib/lsp-framework`, `CMakeLists.txt:845-848`), unlike `Compiler`/`TCCCompiler`.

### 8.2 Run

```bash
# stdio transport (typical editor integration)
cmake-build-debug/ChemicalLsp --stdio

# socket transport (default port 5007)
cmake-build-debug/ChemicalLsp --port 5007

# constrain to a client
cmake-build-debug/ChemicalLsp --stdio --client zed
```

Build scripts are evaluated via the `--build-lab` child mode launched automatically by
`launch_child_build`; you normally never invoke that manually.

### 8.3 Tests

The LSP test harness is `server/tests/LspTests.cpp`, compiled into `ChemicalLsp` and
exposed only in DEBUG builds via `--run-tests` (`LSPMain.cpp:390-395`):

```bash
./scripts/build.sh --lsp
cmake-build-debug/ChemicalLsp --run-tests
```

`run_lsp_tests` (`server/tests/LspTests.cpp:201`) is currently a **formatter-only**
suite: it lexes small snippets and asserts exact formatted output across spacing,
indentation, comments, annotations, arrays, and vertical spacing. There is no automated
end-to-end test for the LSP protocol itself; IDE features are typically verified with a
live client.

## 9. Gotchas, Performance, and Extending

### 9.1 Gotchas

- **Single `LocationManager`, lifetime coupling.** `loc_man` is session-wide
  (`WorkspaceManager.h:153`). Locations encoded for a cached file become stale when the
  file is re-lexed; tokens keep `linked` pointers into AST nodes, so never mutate a
  buffer between lexing and link-back.
- **Link-back by index.** Parsing runs on comment-stripped `copied_tokens`; results are
  copied to the original token vector via `originalIndexes`
  (`LspSemanticTokens.cpp:977-985`). Changing the token-copy scheme breaks semantic
  tokens, hover, and goto-def simultaneously.
- **Editor buffers override disk.** `overriddenSources` wins in `get_lexed`
  (`server/Importer.cpp:27-39`). Features that read the file directly (formatting,
  `get_formatting`) must explicitly fall back to disk, as it does at
  `WorkspaceManager.cpp:338-344`.
- **`chemical.mod` diagnostics are not symbols.** The `.mod` parser populates
  `ModuleFileData`; it does not create Chemical symbols, so hover/completion inside
  `.mod` are limited to what the generic path offers.
- **Whole-document changes.** If `didChange` delivers an unhandled whole-document event
  in a release build it silently mishandles it; DEBUG throws
  (`WorkspaceManager.cpp:546-550`).
- **Async publish races.** Diagnostics publishing is async; the cancel flag and mutex
  must be respected to avoid publishing stale diagnostics out of order.
- **`get_target_triple()` returns `"LSP"`** (`WorkspaceManager.cpp:48-53`) — target
  conditions are a virtual LSP target, not the host triple.
- **`DocumentLinksAnalyzer` is a no-op** — the commented-out body returns an empty
  vector; the capability is not advertised.

### 9.2 Performance

- **Incremental dirty tracking.** `dirtyModules` + `should_process_file`
  (`LspSemanticTokens.cpp:550`) avoid re-resolving untouched dependencies. Single-file
  dirty tracking means one edited file at a time.
- **One file at a time.** `process_file_mutex` serializes symbol resolution, while
  parsing uses the CTPl thread pool (`parseModuleWithDeps`, `:201`). The resolver
  allocators are batched arenas cleared only on re-resolution, so routine requests are
  cheap.
- **LRU caches** bound token (`tokenCache`, cap 10), `.mod` (`modFileData`, cap 10), and
  anonymous (`anonFilesData`, cap 10) memory. Evicted token results are re-lexed on
  demand.
- **Batched AST units** cost ~10 KB each (`parseFile`, `:115`); large projects should
  rely on the LRU eviction and dirty pruning rather than holding everything hot.
- **Cancellation.** `publish_diagnostics_cancel_flag` allows a pending diagnostics task
  to be aborted when a newer request arrives.

### 9.3 Adding a new LSP feature

1. **Advertise the capability** in the `Initialize` handler result
   (`LSPMain.cpp:103-115`), and (if it has triggers) add `CompletionOptions`/
   `SignatureHelpOptions` entries.
2. **Register the handler** in `registerDefaultHandlers` (`LSPMain.cpp`), converting the
   protocol `Position`/`Range` to the manager's `Position` and calling a new
   `WorkspaceManager` method. Guard against exceptions with the existing pattern
   (semantic tokens wraps in try/catch and throws `"UNCAUGHT_EXCEPTION"`,
   `LSPMain.cpp:136-143`).
3. **Add the manager method** in `WorkspaceManager.h`/`.cpp`. Follow the established
   shape: `canonical(path)`, `getModuleData`, `process_file_on_request(...)`, then use
   `get_cached_unit` (`WorkspaceManager.cpp:248`), the cached `LexResult` from
   `tokenCache` for lexical features, or `anonFilesData` for anonymous files.
4. **Implement an analyzer** under `server/analyzers/` and add it to the
   `ChemicalLsp` source list in `CMakeLists.txt:664-716`.
5. If the feature needs compiler-internal information not on the AST, prefer an existing
   intrinsic or extend the symbol-resolution pass — do **not** add a second
   symbol table.
6. **Test** via `server/tests/LspTests.cpp` where a pure-function equivalent exists
   (formatters, tokenizers), and manually via a live client otherwise.

## 10. Related Skills

- **CLI Entrypoint and Targets** (`.agents/skills/cli_entrypoint/SKILL.md`) — `ChemicalLsp` target, `main()`, `TargetData`
- **Build System (LabBuildCompiler)** (`.agents/skills/build_system/SKILL.md`) — jobs, modules, `compile_cbi`
- **Symbol Resolution (symres)** (`.agents/skills/symres/SKILL.md`) — `SymbolResolver`, `tld_declare_file`, `link_signature_file`, `declare_and_link_file`
- **Diagnostics and Error Reporting** (`.agents/skills/diagnostics/SKILL.md`) — `Diag`, `ASTDiagnoser`, severities
- **Compiler Bindings** (`.agents/skills/compiler_bindings/SKILL.md`) — CBI, `CompilerBinder`, enum-sync rule
- **Compiler Plugin API (CBI)** (`.agents/skills/cbi_plugin_api/SKILL.md`) — writing the `*_ide` hooks
- **Performance Optimization** (`.agents/skills/performance/SKILL.md`) — arenas, caching, parallelization
