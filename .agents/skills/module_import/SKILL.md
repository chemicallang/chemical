---
name: Module and Import System
description: Comprehensive guide to the Chemical module and import resolution system — how chemical.mod packages map to build.lab jobs, how local/package/remote/conditional imports become module dependencies, remote git import download and version pinning, link libraries, and the per-module AST records that flow through compilation. Load when working on import resolution, chemical.mod conversion, remote imports, the dependency graph, or module metadata.
---

# Module and Import System

Chemical's module system turns declarative `chemical.mod` / `build.lab` files into a graph of
`LabModule` objects, each compiled independently and linked into a job. This skill documents the
whole path: **module declaration → import resolution → dependency graph → job execution**. The
user-facing syntax lives in [`chemical_mod`](../chemical_mod/SKILL.md); this skill focuses on
*how the compiler implements it*.

## Overview: The Module Model

A build is a tree of **modules**: one compilation unit with a name and scope, its own source
files, its own `ModuleScope` symbol namespace, and a list of dependencies.

| Concept | C++ type | Meaning |
|---------|----------|---------|
| Package kind | `PackageKind` (`compiler/lab/PackageKind.h:7`) | `Library` (main mangled) vs `Application` (main is entry) |
| Module kind | `LabModuleType` (`compiler/lab/LabModuleType.h:5`) | `Files`, `CFile`, `CPPFile`, `ObjFile`, `Directory` |
| Module | `LabModule` (`compiler/lab/LabModule.h:17`) | Name/scope, paths, direct files, dependencies, link info |
| Module namespace | `ModuleScope` (`ast/structures/ModuleScope.h:10`) | `scope_name` + `module_name` + back-pointer to `LabModule` |
| Module registry | `ModuleStorage` (`compiler/lab/ModuleStorage.h:10`) | Owns every `LabModule`, indexed by `scope:name` |
| Dependency edge | `ModuleDependency` (`compiler/lab/import_model/ModuleDependency.h:9`) | `{ LabModule* module; DependencySymbolInfo* info; }` |
| Remote import | `RemoteImport` (`compiler/lab/import_model/RemoteImport.h:15`) | Git repo request + merged requesters + built module cache |
| Dependency symbols | `DependencySymbolInfo` (`compiler/lab/import_model/DependencySymbolInfo.h:8`) | Which symbols an import exposes (alias/items) |
| Import item | `ImportSymbol` (`compiler/lab/import_model/ImportSymbol.h:8`) | One symbol path + alias |

A **module** maps to a **job** only at the top: an executable/library job holds a flat list of
module dependencies, and `ModuleStorage` (per `LabBuildCompiler`) ensures a module built once is
reused.

```
chemical.mod ─(ModToLabConverter)→ build.lab ─(TinyCC JIT)→ build() → LabModule tree
                                          job.dependencies → flatten_dedupe_sorted → [dep, ..., root]
```

### How modules become jobs

1. `build_module_build_file_no_alloc` (`LabBuildCompiler.cpp:3436`) JIT-builds the root
   `.mod`/`.lab` and calls its generated `build(ctx, job)`, which returns the root `LabModule*`
   (`:3493`).
2. `process_remote_imports` (`:4735`) downloads pending git imports and attaches them.
3. `flatten_dedupe_sorted(exe->dependencies)` (`:1643`) yields a dependency-first module vector.
4. Each module is parsed, sym-resolved, type-verified, translated and codegen'd
   (`process_module_tcc`/`process_module_gen`, `:776`/`:1044`). With `--arg-interpret`, an
   `Interpretation` job builds the same graph but interprets `main()` (`do_interpretation_job`,
   `:4788`).

## Key Files

| File | Role |
|------|------|
| `compiler/lab/mod_conv/ModToLabConverter.cpp` | `chemical.mod` → `build.lab` code generation (`convertToBuildLab`, `:89`) |
| `compiler/lab/LabBuildCompiler.cpp` | Build orchestration, `create_module_for_dependency` (`:2301`), `build_module_from_mod_file` (`:2497`), remote imports (`:3830`–`:4786`) |
| `compiler/lab/LabBuildContext.cpp/.h` | `new_module`, `build_exe`, module registration into `ModuleStorage` |
| `compiler/lab/import_model/*` | `ImportSymbol`, `ModuleDependency`, `RemoteImport`, `DependencySymbolInfo` |
| `compiler/lab/LabModule.h` | `LabModule` struct (paths, deps, direct files, options) |
| `compiler/lab/LabModuleOptions.h` | Mirrors the Chemical `ModuleOptions` struct (safety/checks/opt level) |
| `compiler/lab/ModuleStorage.h` | Owns and indexes all modules |
| `compiler/lab/LabJob.h` | Job deps, `remote_imports`, `remote_import_index`, `built_files`, conflict strategy |
| `preprocess/ImportPathHandler.cpp/.h` | `resolve_mod_dep_import` (`:266`), `@` directive + `libs/` resolution |
| `compiler/processor/ModuleFileData.h` | Parsed `.mod` fields (`sources_list`, `link_libs`, `c_files`, ...) |
| `compiler/processor/ASTFileMetaData.h` | Per-file id, module, abs path, import stmt, result |
| `compiler/processor/ASTFileResult.h` | Full file parse/analysis result |
| `compiler/processor/ModuleDependencyRecord.h` | `{ module_dir_path }` intermediate dependency record |
| `compiler/ASTProcessor.cpp` | `determine_module_files` (`:133`), `figure_out_direct_imports` (`:866`), `sym_res_module` dep declaration (`:430`) |
| `compiler/ASTCompiler.cpp` | `shallow_dedupe_sorted` (`:1620`) for direct-dep processing order |
| `lang/libs/lab/src/lab.ch` | Chemical `BuildContext` API: `new_package`, `add_dependency`, `fetch_mod_dependency`, `ImportRepo` |
| `parser/statements/Import.cpp` | Import statement parser (items, alias, `version`/`subdir`/`branch`/`commit`/`orphan`, `if`) |
| `parser/statements/LexStatement.cpp` | `source`/`link`/`link c`/`ship`/`include` mod parsers |

## `chemical.mod` → `build.lab`

`convertToBuildLab(const ModuleFileData&, std::ostream&)`
(`compiler/lab/mod_conv/ModToLabConverter.cpp:89`) is a *source-to-source* code generator. It
walks `ModuleFileData::scope.body.nodes` and emits Chemical code, then that code is JIT-compiled
by TinyCC exactly like a hand-written `build.lab`.

The generated file has a fixed shape — it imports `lab`/`std`, imports each local/package
dependency's `build.lab` under a generated identifier (`__mod_N_stmt`, or `__mod_<alias>` when
aliased), and defines `build()` which creates the package and attaches dependencies:

```chemical
import lab; import std;
import "<path>/build.lab" as __mod_0_stmt;   // relative;  "@<pkg>/build.lab" for packages

public func build(ctx : *mut BuildContext, __chx_job : *mut LabJob) : *mut Module {
    var __curr_lab_path = lab::get_my_path();
    const __chx_already_exists = ctx.get_cached(__chx_job, &__curr_lab_path);
    if(__chx_already_exists != null) { return __chx_already_exists; }
    const deps : []ModuleDependency = [
        ModuleDependency { module: __mod_0_stmt.build(ctx, __chx_job),
                           info: &mut DependencySymbolInfo { ... } },
    ];
    const mod = ctx.new_package(ModuleType.Directory, PackageKind.Application,
        "<scope>", "<name>", std::span<ModuleDependency>(deps, N));
    ctx.set_cached(__chx_job, &__curr_lab_path, mod);
    // then: fetch_mod_dependency | add_path | c_file_module + add_dependency
    //       link_system_lib / add_lib_search_path | ship_file | options | interfaces
    return mod;
}
```

### Conversion rules (with source line refs)

| `.mod` construct | Generated `build.lab` | Ref |
|---|---|---|
| `import "../lib"` | `import "../lib/build.lab" as __mod_N_stmt` + `ModuleDependency` entry | `ModToLabConverter.cpp:98-125`, `:143-166` |
| `import std` (native/pkg) | `import "@std/build.lab" as ...` (note the `@`) | `ModToLabConverter.cpp:107-110` |
| `import { a.b as c } from "..."` | `DependencySymbolInfo { symbols: [ImportSymbol{parts, alias}], alias, location }` | `ModToLabConverter.cpp:54-87` |
| remote import | **skipped** in the import/deps pass; emitted later as `ctx.fetch_mod_dependency` | `ModToLabConverter.cpp:103-106`, `:171-230` |
| `source "src"` | `{ var rel_path = lab::rel_path_to("src"); ctx.add_path(mod, rel_path.to_view()); }` | `ModToLabConverter.cpp:233-249` |
| `link c "f.c"` | `ctx.c_file_module(scope, name_cfile_N, path, ...)` + `ctx.add_dependency(job, c_file_mod, null)` | `ModToLabConverter.cpp:251-306` |
| `link "mylib"` | `ctx.link_system_lib(__chx_job, "mylib", mod)` | `ModToLabConverter.cpp:308-334` |
| `link path "./libs"` | `ctx.add_lib_search_path(__chx_job, rel_path.to_view(), null)` | `ModToLabConverter.cpp:320-328` |
| `ship "res"` | `ctx.ship_file(__chx_job, rel_path.to_view())` | `ModToLabConverter.cpp:336-355` |
| `option safety = ...` | `__mod_opts.<key> = <value>;` | `ModToLabConverter.cpp:357-381` |
| `interface X` | `ctx.add_compiler_interface(mod, "X")` | `ModToLabConverter.cpp:383-388` |

The import identifier is derived by `writeAsIdentifier`
(`ModToLabConverter.cpp:21`): `__mod_<topLevelAlias>` when an alias exists, otherwise
`__mod_<index>_stmt`. Conditions are emitted by `writeIfConditional` (`:35`) as
`__chx_job.getTarget().<flag>` (with `!` and `&&`/`||` supported).

The `get_cached`/`set_cached` pair is what makes the generated `build()` idempotent: two
importers of the same `.mod` path share one `LabModule`.

> There is a second, runtime path: when a `.mod` is imported *by a `.ch` file or another
> `.lab`*, `ASTProcessor::import_mod_file_as_lab` (`compiler/ASTProcessor.cpp:950`) converts it
> in-memory to `build.lab` text and parses that.

## The Dependency Graph

### Import classification

At the AST level, every `import` is an `ImportStatement`
(`ast/statements/Import.h:47`) with `ImportStatementKind` = `NativeLib` or `LocalOrRemote`, plus
remote metadata. Resolution predicates decide its fate:

| Predicate | File:line | True for |
|-----------|-----------|----------|
| `isNativeLibImport()` | `Import.h:155` | `import std` (keyword form) |
| `isFileImport()` | `Import.h:202` | source ends in `.ch`/`.lab`/`.mod` |
| `isRemoteImportPredict()` | `Import.h:162` | has version/commit/branch/subdir, or `http(s)://`/`git@`, or non-relative path |
| `isRemoteModuleImport()` | `Import.h:209` | remote **and** not a `.lab`/`.mod` file |
| `isLocalModuleImport()` | `Import.h:217` | native lib, or relative non-file path |
| `isLocalFileImport()` | `Import.h:226` | local `.ch`/`.lab`/`.mod` file |

Import syntax is parsed in `parser/statements/Import.cpp`: brace `{...} from "src"` form
(`:92`), string `"path" as alias` form (`:126`), identifier form (`:131`), then global alias
(`:170`), remote metadata (`parseImportMetadata`, `:57`), then `if` guard (`:179`).

### Resolution entry point

`ImportPathHandler::resolve_mod_dep_import`
(`preprocess/ImportPathHandler.cpp:266`) is the single dispatcher used by both `.mod` loading
(`LabBuildCompiler.cpp:2651`) and `build.lab` loading (`:2832`). It returns an
`ImportedModuleDepResult` (`ImportPathHandler.h:19`) with either:

- `directory_path` — a local module directory to build, **or**
- `error_message` — hard error, **or**
- **both empty** → skipped (import disabled by its `if` condition).

For a **remote module import** it converts the statement into a `RemoteImportCBI`
(`ImportPathHandler.cpp:287-316`) and calls `BuildContextfetch_mod_dependency` — remote imports
never produce a `directory_path`; they are queued on the job instead.

### Resolving each import type to a concrete module

| Import type | Resolution | Result |
|---|---|---|
| Relative `"../lib"` | `resolve_sibling(base_path, sourcePath)` (`ImportPathHandler.cpp:337-339`) | directory path → `create_module_for_dependency` |
| Native/package `import std` | `resolve_native_lib("std")` → `libs/std` (`ImportPathHandler.cpp:96`, `:332`) | package directory |
| `@` directive `"@system/..."` / `"@std/..."` | `replace_at_in_path` (`:215`), `get_mod_identifier_from_import_path` (`:80`) | system header or `libs/<name>/...` |
| Conditional | `resolve_target_condition` first (`:276-285`) | skipped or resolved |
| Remote git | `BuildContextfetch_mod_dependency` (`:316`) | queued on job, downloaded later |

For local/package import resolution, `create_module_for_dependency`
(`LabBuildCompiler.cpp:2301`) checks in order:

1. `<dir>/build.lab` exists → JIT it and call `chemical_lab_build(&context, job)` (`:2307-2351`).
2. Else `<dir>/chemical.mod` exists → `build_module_from_mod_file` (`:2355-2380`).
3. Else → hard error: *"doesn't contain a 'build.lab' or 'chemical.mod'"* (`:2384`).

Both branches cache the result in `job->built_files` keyed by canonical path
(`:2314-2321`, `:2347-2348`), so the same dependency is never built twice within a job.

`build_module_from_mod_file` (`LabBuildCompiler.cpp:2497`):

1. Parses the `.mod` into a `ModuleFileData` (`import_chemical_mod_file`).
2. `context.new_module(scope_name, module_name, package_kind)` (`:2530`).
3. Appends resolved `source` paths, applying `resolve_target_condition_nullable` (`:2533-2542`).
4. Creates one `CFile` module per `link c` file (with include dirs) and adds it to the **job**
   (`:2558-2591`).
5. Applies link libs / search paths, ship files, and `ModuleOptions` (`:2593-2633`).
6. **Two-phase dependency resolution** (`:2635-2676`): first parse all `ImportStmt`s into
   `ModuleDependencyRecord`s, *then* call `create_module_for_dependency` — because building a
   dependency can clear the module allocator that holds the parsed `.mod` nodes.
7. Adds each `LabModule*` to `module->dependencies` and clears allocators.

### Dependency symbol visibility (which symbols an import exposes)

Each `ModuleDependency` may carry a `DependencySymbolInfo` (`DependencySymbolInfo.h:8`):
`symbols` (selected paths + aliases), `alias` (whole-module alias), `location`. During symbol
resolution, `ASTProcessor::sym_res_module` (`compiler/ASTProcessor.cpp:437-464`) walks direct
dependencies, builds a `ChildrenMapNode` from each dependency module's public nodes, and
`declareChildren` (`:396-412`) decides visibility:

- `dep.info == nullptr` → declare **all** symbols (`declare_or_shadow`).
- `info->alias` non-empty → declare the whole module under that name (does not shadow).
- `info->symbols` non-empty → declare only those symbols via `declareImportedSymbol`.
- otherwise → declare all symbols.

The same logic is implemented for source-level `import` at
`compiler/symres/DeclareTopLevel.cpp:37` (`declareChildren`) and
`TopLevelDeclSymDeclare::VisitImportStmt` (`:57`), which handles `import` results that are
`File` or `Module`.

## Remote Imports

### Syntax

```chmod
import "github.com/owner/repo"
import "owner/repo"                       # origin defaults to github.com
import "github.com/owner/repo" version "1.2.3"
import "github.com/owner/repo" branch "main"
import "github.com/owner/repo" commit "abc1234"
import "github.com/owner/repo" subdir "libs/foo"
import "github.com/owner/repo" orphan branch "feature-x"
import "github.com/owner/repo" version "1.2.3" if posix
```

Recognised metadata keys are `version`, `subdir`, `branch`, `commit`, plus the `orphan`
flag (`parser/statements/Import.cpp:57-85`). An `if` condition may follow the metadata
(`Import.cpp:179`).

### Parsing the repo identity

`parse_remote_import_from` (`LabBuildCompiler.cpp:3830`) splits `from` into `origin`,
`mod_scope`, `mod_name`: `"owner/repo"` and `"github.com/owner/repo"` both give
`origin="github.com"`, `scope="owner"`, `name="repo"`. If scope/name are supplied (from CBI) and
only `origin` is missing, it defaults to `github.com`.

### Registration, dedup and conflict resolution

`add_remote_import` (`LabBuildCompiler.cpp:4370`) builds a key `origin/scope/name`, additionally
including the branch when `orphan_branch` is set and the `subdir` (`:4377-4396`). Notably,
**`version` is not part of the key** — that is what lets two different versions collide and be
resolved by policy.

- New key → heap-allocated `RemoteImport` in `job->remote_imports`, indexed by
  `job->remote_import_index` (`:4427-4430`).
- Existing key → `resolve_remote_import_conflict` (`:4434`); requesters are **merged** so every
  importer links to the single built module (`:4414-4423`). If already built
  (`built_module != null`), requesters attach immediately (`:4405-4412`).

`RemoteImport` (`RemoteImport.h:15`) holds `requesters` (`RemoteImportRequester`: requesting
`LabModule*` + `DependencySymbolInfo*`) and the cached `built_module`.

`ConflictResolutionStrategy` (`LabJob.h:30`): `Default`, `PreferNewerVersion` (job default),
`PreferOlderVersion`, `RaiseError`, `OverridePrevious`, `KeepPrevious`. Versions compare via
`compare_remote_versions` (`:3799`) — a `v`-prefix-tolerant numeric dotted comparison returning
`-2` for non-semver strings, which falls back to `RaiseError` (`:4466-4470`).

### Download and caching

`download_remote_import` (`LabBuildCompiler.cpp:4614`):

- Storage dir: `<job.build_dir>/remote/<origin>/<scope>/<name>[@branch][@version][@commit]`
  (`get_remote_repo_info`, `:3880`); orphan branches get an `@branch` suffix (`:3890-3893`). If
  the dir already exists, nothing is downloaded (`:4646`).
- Otherwise it shells out to `git` (`run_git_and_capture`, `:4581`), prefixing `https://` unless
  the URL already has a scheme or is `git@` (`:4649-4652`). Shallow clones:
  `clone --quiet --depth 1 [--branch <version-or-branch>]`; for a `commit` instead
  `init`+`remote add`+`fetch --depth 1 <sha>`+`checkout FETCH_HEAD` (`:4663-4677`).
- Non-interactive (`GIT_TERMINAL_PROMPT=0 GIT_ASKPASS=true SSH_ASKPASS=true`, stdin from
  `/dev/null`, `:4655-4660`); `subdir` selects `<target>/<subdir>` (`:4694-4699`).
- The downloaded module is built via `create_module_for_dependency`, cached in
  `import->built_module`, and attached to every requester (`:4701-4730`).

`process_remote_imports` (`:4735`) processes imports in **waves** so newly-discovered transitive
remote imports are handled next iteration. It uses a thread pool, a progress bar, and a
per-target-dir `download_mutex` + condition variable to avoid concurrent downloads
(`:4627-4644`).

For `chemical run <remote>` / `local_or_remote_project_to_module`, downloaded sources live under
`~/.chemical/commands/...` (`get_commands_cache_dir`, `:3520`) and the build dir is deleted after
a successful run (`run_job`, `:3941`).

## Conditional Dependencies and Target-Specific Deps

`if` conditions parse into an `IffyBase` tree (`ModFileIfBase` in
`compiler/processor/ModuleFileData.h:15`). They appear on `source`, `import`, `link`, `link c`,
`ship`, and `include`. Flags come from `TargetData` (`compiler/lab/TargetData.h:7`) and mirror
the Chemical `TargetData` (`lang/libs/lab/src/target_data.ch`): `windows`, `linux`, `macos`,
`posix`, `x86_64`, `aarch64`, `tcc`, `clang`, `debug`, `release`, `test`, `cbi`, `lsp`, etc.

Two evaluation paths exist:

- **In generated `build.lab`** — `writeIfConditional` emits
  `if(__chx_job.getTarget().<flag>) { ... }`, evaluated by the build script at runtime
  (`ModToLabConverter.cpp:35`).
- **In `build_module_from_mod_file`** — `resolve_target_condition_nullable` runs in C++
  (`LabBuildCompiler.cpp:2534`, `:2561`, `:2595`, `:2615`; `resolve_target_condition` in
  `compiler/lab/LabJob.cpp:99`). An unresolvable condition (e.g. a bogus flag) is an error.

`resolve_mod_dep_import` also evaluates the import's own `if_condition` and returns a *skipped*
result when false (`ImportPathHandler.cpp:276-285`). This is how the same `.mod` can list
`source "win" if windows` / `source "posix" if !windows` and only one is compiled.

## Link Libraries, C Files, Includes, Ship Files

These attach to the **job** (or a dedicated C module), not to the Chemical module being
compiled:

- `link "name"` → `ctx.link_system_lib(job, name, mod)`, appended to `job->link_libs`
  (`LabBuildCompiler.cpp:2594-2606`). Chemical uses `:libfoo.so.0` to link an exact soname.
- `link path "./dir"` → `job->lib_search_paths` (`:2603-2605`).
- `link c "file.c"` (`parser/statements/LexStatement.cpp:621-709`) → a synthetic
  `LabModuleType::CFile` module named `<module>_cfile_<N>` with its path and include dirs, added
  to the job as a dependency (`LabBuildCompiler.cpp:2558-2591`). C files are compiled to separate
  objects and linked; the compiler does **not** parse them, so declare functions with `@extern`.
- `include "dir"` → `ModuleFileData::include_dirs`, copied onto C-file modules (`:2569-2586`).
- `ship "path"` → `job->ship_files`, copied to the output dir (`:2613-2624`).
- `option key = value` → `LabModuleOptions` (mirror of `ModuleOptions` in `lab.ch:20-26`), applied
  by `apply_module_options` via an FNV-1a key switch (`:2448-2495`, `:2630-2633`).

> `LabModuleOptions` and Chemical's `ModuleOptions` **must stay field-for-field in sync**
> (`compiler/lab/LabModuleOptions.h:23`, `lang/libs/lab/src/lab.ch:20`), since the struct is
> crossed through CBI.

## Dependency Graph Traversal, Dedup and Cycles

### Flattening (transitive, dependency-first)

`flatten_dedupe_sorted` (`LabBuildCompiler.cpp:156`) walks `ModuleDependency` edges with
`recursive_dedupe` (`:141`): it recurses into every dependency first, then appends the module if
not already seen. Result: a flat vector where a module always appears **after** everything it
depends on. Used by `process_job_tcc`/`process_module_gen` and `do_interpretation_job`.

`shallow_dedupe_sorted` (`compiler/ASTCompiler.cpp:1620`) is the codegen-time variant: it only
emits modules that are among the requested direct dependencies (transitives are used for
ordering but not emitted separately), preventing a dependency's code from being emitted twice.

### Dedup points

| Mechanism | Scope | File:line |
|---|---|---|
| `flatten_dedupe_sorted` / `recursive_dedupe` | module pointer | `LabBuildCompiler.cpp:141-163` |
| `job->built_files` (canonical path) | `.mod`/`.lab` build | `LabBuildCompiler.cpp:2314`, `:2347` |
| `remote_import_index` | remote repo identity | `LabJob.h:129`, `LabBuildCompiler.cpp:4398` |
| `RemoteImport::built_module` | already-built remote | `RemoteImport.h:36` |
| `ModuleStorage` index | `scope:name` globally | `ModuleStorage.h:22` |
| `ASTFileResult` cache (`cache` map) | file id | `ASTProcessor` (`:825`) |
| `ASTFileResult::imports` | per-file direct imports | `ASTFileResult.h:33` |

### Circular dependencies

Cycle detection is **file-level**, over the import tree of `build.lab` files:
`check_imports_for_cycles` (`LabBuildCompiler.cpp:272-336`) walks `ASTFileResult::imports` and
reports direct (`a imports a`) and indirect cycles (via the `parents` stack), printing each
link in the chain. A detected cycle aborts the build (`:2785`). Recursion in
`check_imports_for_cycles` is guarded against infinite descent once a cycle is found (`:295`).

Module-level cycles are avoided implicitly: the generated `build()` uses
`ctx.get_cached`/`ctx.set_cached`, and `create_module_for_dependency` caches by canonical path,
so re-entrant imports reuse the in-progress module pointer. There is **no separate module-graph
cycle diagnostic** — if you construct one that bypasses the cache, expect a stack overflow.

### Transitive remote imports

`process_remote_imports` loops in waves: building a downloaded module may call
`ctx.fetch_mod_dependency`, appending to `job->remote_imports`; the loop re-scans from
`processed_count` and processes the new entries (`LabBuildCompiler.cpp:4744-4783`).

## Per-Module Records and Metadata Flow

### `ModuleFileData` — the parsed `.mod`

`ModuleFileData` (`compiler/processor/ModuleFileData.h:79`) holds the parser output for a
`chemical.mod`: `scope_name`, `module_name`, `package_kind`, `sources_list` (`ModFileSource`),
`link_libs` (`ModFileLinkLib`), `c_files` (`ModFileCFile`), `include_dirs`, `ship_files`,
`compiler_interfaces`, `options` and diagnostics. `ModuleFileDataUnit` (`:187`) bundles an
`ASTAllocator` + `ASTFileMetaData` + `ModuleFileData` and is reusable via `clear()`.

### `ASTFileMetaData` and `ASTFileResult` — per source file

`ASTFileMetaData` (`compiler/processor/ASTFileMetaData.h:16`) carries `file_id`, the owning
`ModuleScope* module`, `abs_path`, the `ImportStatement* stmt` that pulled the file in, the
private symbol range, and a back-pointer to `ASTFileResult* result`.

`ASTFileResult` (`compiler/processor/ASTFileResult.h:12`) extends it with the parsed `ASTUnit`,
`imports` (deduped direct imports as `ASTFileMetaData`), the `sig_result`, diagnostics and
benchmarks. It is the unit of parallel work in `ASTProcessor`.

### `ModuleDependencyRecord` — the intermediate edge

`ModuleDependencyRecord` (`compiler/processor/ModuleDependencyRecord.h:13`) is just
`{ std::string module_dir_path; }`. It is produced by `resolve_mod_dep_import` *before* the
module directory is actually built, so that building a dependency (which may clear allocators)
can happen after all import statements have been parsed (`LabBuildCompiler.cpp:2639-2676`).

### Metadata flow through compilation passes

```
chemical.mod
   │  import_chemical_mod_file → ModuleFileData
   ▼
build_module_from_mod_file → LabModule + dependencies (LabModule*)
   ▼
ASTProcessor::determine_module_files(mod)   # ASTProcessor.cpp:133
   │  Files/Directory → direct_files: vector<ASTFileMetaData>
   ▼
import_module_files_direct → import_chemical_file_recursive  # ASTProcessor.cpp:1038
   │  lex+parse each file; figure_out_direct_imports (# :866) fills ASTFileResult::imports
   ▼
sym_res_module  # ASTProcessor.cpp:430
   │  1. declare direct dependency children (DependencySymbolInfo visibility)
   │  2. sym_res_tld_declare_file  (private_symbol_range)
   │  3-6. link sig / gen inst / after-sig / generic bodies / full bodies
   ▼
type_verify_module_parallel → declare_module/implement_module → codegen
```

`determine_module_files` is where a module's source set becomes concrete
(`ASTProcessor.cpp:133`): `Files` treats each `paths` entry as a `.ch` file (`:144`),
`Directory` globs each directory (or single `.ch`) for `.ch` files (`:161-204`), and
`ObjFile`/`CFile`/`CPPFile` have no Chemical direct files (`:157`).

`figure_out_direct_imports` (`:866`) only handles `isLocalFileImport()`; it resolves paths via
`path_handler.resolve_import_path`, and for `@`-prefixed imports determines the owning module
through `mod_storage.find_module(atDirective.replaced)` (`:909`). Symbols declared from an
external `@` module are at least `Public`, whereas plain relative files are `Internal`
(`DeclareTopLevel.cpp:74-79`).

## Gotchas

- **A module directory must contain `build.lab` or `chemical.mod`.** `create_module_for_dependency`
  hard-errors otherwise (`LabBuildCompiler.cpp:2384`); relative imports resolve to a *directory*.
- **`import cstd` in a `.mod` becomes `import "@cstd/build.lab"` in generated code.** The leading
  `@` is the package marker, not the `@`-directive import form.
- **Two-phase resolution is mandatory.** `create_module_for_dependency` can clear the module
  allocator holding parsed `.mod` nodes, so collect `ModuleDependencyRecord`s first, then build
  (`LabBuildCompiler.cpp:2635-2676`).
- **`version` is not part of the remote import dedup key** (`:4377-4396`). Different versions of
  one repo merge into a single `RemoteImport` and are resolved by `ConflictResolutionStrategy`
  (default `PreferNewerVersion`); non-semver versions raise (`compare_remote_versions` → `-2`).
- **Orphan branches and `commit` are special-cased.** Orphan branches are namespaced in the dedup
  key (`:4386`) and storage path (`@branch`, `:3890`); a `commit` switches to
  `init`+`fetch --depth 1 <sha>`+`checkout FETCH_HEAD` instead of a shallow clone (`:4663-4676`).
- **`LabModuleOptions` and Chemical's `ModuleOptions` must stay field-for-field in sync**
  (`LabModuleOptions.h:8-13`, `lang/libs/lab/src/lab.ch:20`) — the struct crosses CBI.
- **Cycle detection is file-level only** and only runs for `build.lab` import trees
  (`LabBuildCompiler.cpp:2782-2790`). Module-level cycles rely on cache-based re-entrancy.
- **`source` paths are relative to the `.mod`/`.lab` file** (`resolve_sibling`, e.g.
  `LabBuildCompiler.cpp:2537`), while build.lab path variables use `lab::rel_path_to`.
- **The import item list controls symbol visibility** — empty info means "all symbols", whereas
  `{ a, b }` imports only those (`declareChildren`, `ASTProcessor.cpp:396`).

## Worked Example: Multi-Module Project with a Remote Import

### Layout

```
my_app/chemical.mod + src/main.ch
my_app/libs/greet/chemical.mod + src/greet.ch
```

### `my_app/chemical.mod`

```chmod
application my_app

source "src"

# local module
import "./libs/greet"

# remote module, pinned, only on posix, exposing just two symbols
import "chemicallang/example" version "v1.2.0" if posix
import { math.clamp as clamp, math.pi } from "github.com/owner/repo"  # symbol-selective

# native package
import std

link "m"
link c "helper.c" { include "vendor/include" }
```

### `my_app/libs/greet/chemical.mod`

```chmod
module greet

source "src"

import cstd
import std

link "m"
```

### What the converter generates (abridged)

```chemical
import "./libs/greet/build.lab" as __mod_0_stmt;
import "@std/build.lab"         as __mod_1_stmt;
// deps: __mod_0_stmt.build(...) + __mod_1_stmt.build(...)
// mod = ctx.new_package(Directory, Application, "", "my_app", deps)
if(__chx_job.getTarget().posix) {
    ctx.fetch_mod_dependency(__chx_job, mod, ImportRepo {
        from: "chemicallang/example", version: "v1.2.0",
        location: intrinsics::get_raw_location() });
}
ctx.fetch_mod_dependency(__chx_job, mod, ImportRepo {
    from: "github.com/owner/repo",
    symbols: [ ImportSymbol { parts: ["math","clamp"], alias: "clamp" },
               ImportSymbol { parts: ["math","pi"],    alias: "" } ],
    location: intrinsics::get_raw_location() });
// then: add_path("src"), c_file_module("helper.c") + add_include_dir + add_dependency,
//       link_system_lib("m", mod)
```

### What happens at build time

1. The root `build()` runs; `libs/greet` and `std` are built recursively (each with its own
   `build.lab`, cached in `built_files`).
2. `fetch_mod_dependency` registers the two remote imports; `chemicallang/example` only because
   `posix` is true.
3. `process_remote_imports` clones `<build>/remote/github.com/chemicallang/example@v1.2.0` and
   `<build>/remote/github.com/owner/repo`, builds each, and attaches it to `mod` (or the job for
   the CLI-form import).
4. `flatten_dedupe_sorted` yields `[cstd, std, greet, example, repo, my_app]` (dependency-first).
5. Each module is parsed, sym-resolved, type-verified and codegen'd; `helper.c` becomes its own
   `CFile` module; `-lm` reaches the linker.

## Related Skills

- [`chemical_mod`](../chemical_mod/SKILL.md) — user-facing `chemical.mod` syntax.
- [`build_system`](../build_system/SKILL.md) — `LabBuildCompiler`, jobs, `ASTProcessor`, TinyCC JIT.
- [`symres`](../symres/SKILL.md) — how declared dependency symbols are linked.
- [`compiler_bindings`](../compiler_bindings/SKILL.md) — CBI, through which `BuildContext` is exposed to `build.lab`.
- [`testing`](../testing/SKILL.md) — how test modules are wired via imports.