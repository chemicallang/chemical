---
name: chemical.mod
description: The syntax and API of chemical.mod file
---

# chemical.mod

A chemical.mod file exists as syntactic sugar for build.lab files, lab files
tell our compiler which jobs to execute and which modules each job depends on

chemical.mod does the same, but its very easy to write and compiler takes care of generating the code

Not always can we write a chemical.mod, When the input has to be dynamic (which file to compile), You must write a build.lab, which is harder to write but provides full control.

A chemical.mod file is basically static information about a module, which modules it depends on, which libraries it wants to link into the final exectuable and so on...

Lets look at its syntax

### Package Type

You start by defining the type of your package, You can use `module` or `application`

The `application` here means that the module contains the main method which should NOT
be mangled. But still the module must contain a public main function, if it doesn't linker
error would occur.

 `my_mod` is ofcourse name of the module

```chmod
application my_mod
```

The `module` keyword here means this is a library and its main function would be mangled (if any) so that it doesn't conflict with other libraries.

```chmod
module my_mod
```

If you run `chemical run chemical.mod` on a file that has `module` and contains a main function, it does NOT mangle the main function, allowing you to compile and run the module itself which was supposed to be library.

We use this approach to allow users to run modules like this

```bash
chemical run chemical.mod --my-first-arg --my-second-arg
```

The arguments to the program which is compiled from the `chemical.mod`

#### Package declaration reference

`parseModuleDefinition` (`parser/statements/LexStatement.cpp:437`) accepts exactly three
leading keywords and maps them onto `PackageKind` (`compiler/lab/PackageKind.h:7`):

| Keyword | `PackageKind` | Meaning | main mangled? |
|---------|---------------|---------|---------------|
| `application <name>` | `Application` | Executable entry point | **No** (`no_mangle`) |
| `module <name>` | `Library` | Compiled as a library | Yes |
| `library <name>` | `Library` | Synonym for `module` | Yes |

`module` and `library` are treated identically by the parser (`LexStatement.cpp:439`).
`PackageKind` has only two values — `Library` and `Application` (`PackageKind.h:7-11`) —
`library` is just an accepted spelling. A module may declare a scope with `.` or `::`
(`LexStatement.cpp:455-471`), giving `scope_name` + `module_name`:

```chmod
module my_scope.my_lib
module my_scope::my_lib
```

Without a separator, `scope_name` is empty. Both are emitted into
`ctx.new_package(..., "<scope>", "<name>", ...)`. There is **no module-level `version`
directive** — versions only exist on `import` statements (see [Version Pinning](#version-pinning)).

### Specifying Source Paths

You must add chemical sources the module requires to be compiled.

```chmod
application my_mod

source "src"
```

The `source` command takes the path, and includes that path, If the path
1 - points to a directory, we find all the chemical source files (.ch extension) recursively and add it to compilation
2 - points to a file, we add that file for compilation (.ch extension)

Parsed by `parseSourceStmt` (`parser/statements/LexStatement.cpp:588`); the path is
relative to the `.mod` file.

#### Multiple Source Directories and Conditional Paths

`source` may be repeated as many times as needed; each entry is appended to
`ModuleFileData::sources_list` (`compiler/processor/ModuleFileData.h:106`). Suppose you want
to include a directory only on windows, because it uses windows APIs

```chmod
source "src"
source "win" if windows
source "posix" if posix
```

Here `win` directory will only be added on windows, `posix` on posix systems, and `src` on both. This allows us to be cross platform, we don't have preprocessor, although we do have conditional comptime if statements that allow you to write platform specific code.

The real `std` module layers shared and platform sources this way:

```chmod
module std

source "src"
source "win" if windows
source "posix" if !windows

import cstd
import core
```

### Specifying Module Imports

To specify module imports, we use the keyword import

##### Relative Imports of Directories

```chmod
application my_mod


import "../lib_mod"

import "../lib_mod2"

```

Note that the path is relative to the chemical.mod file, Chemical expects that these directories contain a `chemical.mod` file or a `build.lab` file.

If it finds a `chemical.mod` file, it will use it to build the module, if it finds a `build.lab` file, it will use it to build the module.

If it finds neither, it will throw an error.

##### Native / package imports

A bare identifier imports a bundled library from `lang/libs/` (or the configured library
search path):

```chmod
import cstd
import std
import core
```

The converter rewrites these to the package form with an `@` prefix — `import std` becomes
`import "@std/build.lab"` (`ModToLabConverter.cpp:107-110`, see
[How the converter maps each directive](#how-the-converter-maps-each-directive)).

##### Conditional Imports

```chmod
import "../lib_mod" if windows
import "../lib_mod2" if posix
```

Here `../lib_mod` will only be imported on windows, `../lib_mod2` on posix systems, and both on both.

#### Remote Imports

You can import modules from github like this

```chmod
import "github.com/owner/repo"
```

This will download the repository and compile it as a module, It will look for a `chemical.mod` file or a `build.lab` file in the root of the repository.

##### Orphan Braches for Remote Imports

If you want to import a module from an orphan branch, you can do it like this

```chmod
import "github.com/owner/repo" orphan branch "branch" if windows
```

In this case the `if` goes at the end. No conflicts are caused with other branches, since orphan branch is considered unique in comparison.

##### Subdirectories in a MonoRepo

If you want to import a module from a subdirectory of a repository, you can do it like this

```chmod
import "github.com/owner/repo" subdir "subdirectory"
```

This will download the repository and compile it as a module, It will look for a `chemical.mod` file or a `build.lab` file in the subdirectory of the repository.

Note that the entire repo would be downloaded for that single directory, This is only
useful if many tiny libraries interop with each other, and if user requires one, he is very likely to need others.

For example, We ship sokol bindings this way, Sokol repo contains all the libraries in a single repo, dividing them into multiple repos would cause difficulty in maintainance.

##### Version Pinning

You can pin the version of the module like this

```chmod
import "github.com/owner/repo" version "1.0.0"
```

This will download the repository and compile it as a module, It will look for a `chemical.mod` file or a `build.lab` file in the root of the repository.

If there are two versions of the same library, A conflict occurs, compiler will try to resolve the conflict by parsing the versions, which it expects to be semantic versions.

If one version is greater than the other, The newer version is kept and older is discarded

The full import metadata grammar (`parser/statements/Import.cpp`) keys are `version "x"`,
`branch "x"`, `commit "sha"`, `orphan branch "x"`, `subdir "x"`, and a trailing `if <cond>`
(always **last**). Remote resolution, conflict policy, and the download cache are covered in
depth by the [`module_import`](../module_import/SKILL.md) skill.

### Linking Libraries

Sometimes you want to link a dynamic library, For this very usecase you must do

```chmod

link path "./my_libs"

link "my_lib"

```

Here chemical will look for `my_lib` inside the `my_libs` directory, or any other directories that have been added to the link search path.

You can write if conditionals as usual on link statements.

#### Linking C Files

There are a lot of single header or single source c files that are easily linkable, which is what we'd do now.

```chmod
link c "my_c_file.c"
```

Here the c file would be compiled and linked into the final program, please note that chemical doesn't parse the c file or any headers, It doesn't know what you just imported, so you must write extern function declarations like this

```chemical
@extern
public func my_c_file_sum_func(a : int, b : int) : int
```

In this case `my_c_file_sum_func` in not mangled and expected to exist during linking.

The c files are compiled to individual object files and then linked with the final program.

A `link c` statement can carry a braced block of include directories (`parseCFileStmt`,
`parser/statements/LexStatement.cpp:678-724`):

```chmod
link c "vendor/helper.c" {
    include "vendor/include"
    include "vendor/other" if windows
}
```

You can also declare module-wide include directories with the top-level `include`
directive; parsed directly in `parseModuleFile` (`parser/Parser.cpp:161-175`) and stored in
`ModuleFileData::include_dirs`:

```chmod
include "include"
include "3rdparty/everest/include"
```

## Full Directive Reference

Every directive is parsed by `BasicParser::parseModuleFile` (`parser/Parser.cpp:120-217`).
The complete set is:

| Directive | Syntax | Parsed by | Stored in `ModuleFileData` |
|-----------|--------|-----------|----------------------------|
| package decl | `application NAME` / `module NAME` / `library NAME` / `SCOPE.NAME` | `LexStatement.cpp:437` | `package_kind`, `scope_name`, `module_name` |
| `source` | `source "path" [if COND]` | `LexStatement.cpp:588` | `sources_list` |
| `import` | see [import forms](#specifying-module-imports) | `parser/statements/Import.cpp` | `scope.body.nodes` (`ImportStmt`) |
| `link` | `link "name" [if COND]` | `LexStatement.cpp:621` | `link_libs` (`Kind::Name`) |
| `link path` | `link path "dir" [if COND]` | `LexStatement.cpp:621` | `link_libs` (`Kind::Path`) |
| `link c` | `link c "file.c" [{ include "..." }] [if COND]` | `LexStatement.cpp:678` | `c_files` |
| `include` | `include "dir" [if COND]` | `Parser.cpp:161` | `include_dirs` |
| `ship` | `ship "path" [if COND]` | `LexStatement.cpp:726` | `ship_files` |
| `interface` | `interface Name;` | `Parser.cpp:189` | `compiler_interfaces` |
| `option` | `option key = value` | `parser/statements/OptionStmt.cpp:15` | `options` |

Directive keywords are matched via `std::hash` on the token value (`Parser.cpp:134-135`), so
they are reserved identifiers at the top level of a `.mod` file.

Each directive in detail:

- **`source`** — adds a directory (recursively globbed for `.ch`) or a single `.ch` file.
  Repeatable; `if` guards apply. See [Specifying Source Paths](#specifying-source-paths).
- **`import`** — pulls in a local directory, bundled package, or remote git module. See
  [Specifying Module Imports](#specifying-module-imports). (The `import { a } from "..."` form
  parses and the converter can emit `DependencySymbolInfo`, but real `.mod` files only use
  the plain/remote forms.)
- **`link "name"`** — links a system library (`ctx.link_system_lib`), with optional `if`.
- **`link path "dir"`** — adds a library search directory (`ctx.add_lib_search_path`), with
  optional `if`.
- **`link c "file.c"`** — compiles a C file to an object and links it. Optional
  `{ include "..." }` adds C-file-local include dirs (`ctx.c_file_module` +
  `ctx.add_dependency`).
- **`include "dir"`** — module-wide C/C++ include directory (`ctx.add_include_dir`).
- **`ship "path"`** — copies a file/dir to the output directory (`ctx.ship_file`).
- **`interface Name`** — registers a CBI compiler interface (`ctx.add_compiler_interface`).
  See [`cbi_plugin_api`](../cbi_plugin_api/SKILL.md).
- **`option key = value`** — validated module option, converted to `mod.getOptions().key`
  (see the key table below). `ship "res/icons" if posix`, `interface BuildContext` and
  `option safety = true` are all valid standalone statements.

### `option` value keys

Validated against the registry in `compiler/ModuleOptionRegistry.cpp:26-37`:

| Option key | Value kind | Allowed values |
|------------|-----------|----------------|
| `safety` | boolean | `true` / `false` |
| `checks.bounds` | boolean | `true` / `false` |
| `checks.overflow` | boolean | `true` / `false` |
| `checks.null` | boolean | `true` / `false` |
| `optimization_level` | integer | `0`–`3` |
| `safe_mode` | string | `none`, `standard`, `strong`, `all` |
| `stack_protector` | string | `none`, `standard`, `strong`, `all` |

Converted to field writes on `mod.getOptions()` (`ModToLabConverter.cpp:357-381`). These
fields must stay in sync with `LabModuleOptions` and Chemical's `ModuleOptions`
(`lang/libs/lab/src/lab.ch`) — the struct crosses CBI.

## Conditional / Target-Specific Directives

`if` conditions may follow `source`, `import`, `link`, `link c`, `include`, and `ship`.
Grammar (parsed by `parseIffyConditional`, `parser/statements/LexStatement.cpp:513`): a bare
flag (`if windows`), negation (`if !windows`), conjunction (`and`/`&&`), disjunction
(`or`/`||`), flattened left-to-right (`:560-584`).

Flags come from `TargetData` (`compiler/lab/TargetData.h:7`) and are read at build-script
runtime as `__chx_job.getTarget().<flag>` (`ModToLabConverter.cpp:35`):

| Group | Flags |
|-------|-------|
| Compiler / job | `c`, `tcc`, `clang`, `cbi`, `lsp`, `test` |
| Mode | `debug`, `debug_quick`, `debug_complete`, `release`, `release_safe`, `release_small`, `release_fast` |
| OS | `posix`, `gnu`, `windows`, `win32`, `win64`, `linux`, `macos`, `freebsd`, `unix`, `android`, `cygwin`, `mingw32`, `mingw64`, `emscripten`, `musl` |
| Arch / misc | `x86_64`, `x86`, `i386`, `arm`, `aarch64`, `riscv`, `riscv32`, `riscv64`, `wasm32`, `wasm64`, `is64Bit`, `little_endian`, `big_endian` |

Real-world example — the `mongodb` wrapper picks a per-platform orphan branch
(`lang/compiled/mongodb/chemical.mod`):

```chmod
module mongodb
source "src"
import std
import "chemicallang/mongodb" orphan branch "win-x64"       if windows and !arm
import "chemicallang/mongodb" orphan branch "linux-x64"     if linux and !arm
import "chemicallang/mongodb" orphan branch "linuxmusl-x64" if linux and musl and !arm
import "chemicallang/mongodb" orphan branch "macos-x64"     if macos and !arm
```

## Module Names, Versions and Nested Modules

- **Name / scope** — from the package declaration (`module scope.name`), or empty scope for a
  single identifier. Emitted to `ctx.new_package` and used to build the generated import
  identifier.
- **Version** — there is no module-level version directive. Version pinning lives only on
  `import ... version "x"` and is resolved by the remote-import conflict policy (see
  [`module_import`](../module_import/SKILL.md)).
- **Nested modules** — a nested package is just a directory with its own `chemical.mod`
  (or `build.lab`) imported via a relative path, e.g. `import "./libs/greet"`. There is no
  special nesting syntax. Module directories are resolved by
  `create_module_for_dependency` (`compiler/lab/LabBuildCompiler.cpp:2301`), which requires
  the directory to contain `build.lab` or `chemical.mod`.

## How the Converter Maps Each Directive

`convertToBuildLab(const ModuleFileData&, std::ostream&)`
(`compiler/lab/mod_conv/ModToLabConverter.cpp:89`) is a source-to-source code generator: it
walks `ModuleFileData::scope.body.nodes` and emits Chemical code that TinyCC JIT-compiles
exactly like a hand-written `build.lab`.

| `.mod` construct | Generated `build.lab` | Ref |
|---|---|---|
| `import "../lib"` | `import "../lib/build.lab" as __mod_N_stmt` + `ModuleDependency` entry | `:98-125`, `:143-166` |
| `import std` (native/pkg) | `import "@std/build.lab" as ...` (note the `@`) | `:107-110` |
| import alias / symbol items | `DependencySymbolInfo { alias, symbols: [ImportSymbol{parts, alias}], location }` | `:54-87` |
| remote import | **skipped** in the import/deps pass; emitted later as `ctx.fetch_mod_dependency` | `:103-106`, `:171-230` |
| `source "src"` | `{ var rel_path = lab::rel_path_to("src"); ctx.add_path(mod, rel_path.to_view()); }` | `:233-249` |
| `link c "f.c"` | `ctx.c_file_module(scope, name_cfile_N, path, ...)` + `ctx.add_dependency(job, c_file_mod, null)` | `:251-306` |
| `include "dir"` | `ctx.add_include_dir(c_file_mod_N, rel_path.to_view())` | `:264-296` |
| `link "mylib"` | `ctx.link_system_lib(__chx_job, "mylib", mod)` | `:308-334` |
| `link path "./libs"` | `ctx.add_lib_search_path(__chx_job, rel_path.to_view(), null)` | `:320-328` |
| `ship "res"` | `ctx.ship_file(__chx_job, rel_path.to_view())` | `:336-355` |
| `option safety = ...` | `__mod_opts.<key> = <value>;` | `:357-381` |
| `interface X` | `ctx.add_compiler_interface(mod, "X")` | `:383-388` |

The generated file begins with `import lab; import std;`, then one
`import "<dep>/build.lab" as __mod_N_stmt;` per local/package dependency, then defines
`build(ctx, __chx_job)` which calls `ctx.new_package(...)`, emits the directive calls above,
and returns the module. Dependency identifiers are `__mod_<topLevelAlias>` when aliased, else
`__mod_<index>_stmt` (`writeAsIdentifier`, `:21`); conditions become
`if(__chx_job.getTarget().<flag>)` (`writeIfConditional`, `:35`). The `get_cached`/`set_cached`
pair makes `build()` idempotent: two importers of the same `.mod` path share one `LabModule`.

> When a `.mod` is imported *by a `.ch` file or another `.lab`*,
> `ASTProcessor::import_mod_file_as_lab` (`compiler/ASTProcessor.cpp:950`) converts it
> in-memory to `build.lab` text and parses that.

Full pipeline: [`build_system`](../build_system/SKILL.md) and
[`module_import`](../module_import/SKILL.md).

## Real-World Examples

### Library with platform source split — `lang/libs/std/chemical.mod`

```chmod
module std

source "src"
source "win" if windows
source "posix" if !windows

import cstd
import core
```

### Library with conditional link + many native deps — `lang/libs/tls/chemical.mod`

```chmod
module tls

source "src"
source "win" if windows

import cstd
import std
import net
import crypto
import encoding
import datetime
import osrand

link "bcrypt" if windows
```

### Application with pinned remote imports — `lang/compiled/timeline/chemical.mod`

```chmod
application main

source "src"

import std
import page
import html_cbi
import css_cbi
import js_cbi
import universal_cbi
import components
import net
import json
import fs
import "github.com/wiqis/accountlib" version "v1.1"
import "github.com/chemicallang/mongodb" version "v1.4"
import "github.com/chemicallang/mdi-icons"
```

### C-library binding with `link c` + `include` — `lang/compiled/mbedtls/chemical.mod` (abridged)

```chmod
module mbedtls

source "src"

import std

include "include"
include "3rdparty/everest/include"

link "ws2_32" if windows

// Core library — 100+ C translation units
link c "library/aes.c"
link c "library/aesce.c"
link c "library/aesni.c"
// ...
link c "library/x509_crt.c"
link c "c/ssl_http_client.c"
```

## Common Patterns

- **Library with link deps** — `module mylib; source "src"; import std; link "m"; link path "./vendor/lib"; link "mylib_native"`.
- **Application with remote imports** — `application my_app; source "src"; import "github.com/owner/repo" version "1.2.0"`.
- **Compiler plugin (CBI)** — `module my_plugin; source "src"; import compiler; interface MyPluginContext`.
- **Large C binding** — one `source` for the Chemical shim, `include` for module-wide headers, one
  `link c` per translation unit, and `link "..." if windows` for platform libraries.

## Gotchas

- **`define` does not exist in `chemical.mod`.** It is a `build.lab`-only API
  (`ctx.define`/`ctx.undefine`). Write a `build.lab` if you need custom build definitions.
- **`if` goes at the end** of an import statement, after all metadata
  (`import "..." orphan branch "x" if windows`). The parser reads metadata first, then the
  guard (`Import.cpp`).
- **A module directory must contain `build.lab` or `chemical.mod`.** Relative imports resolve
  to a directory; `create_module_for_dependency` hard-errors otherwise
  (`LabBuildCompiler.cpp:2384`).
- **`source`, `link c`, `include`, and `ship` paths are relative to the `.mod` file**, not to
  the compile working directory. The converter wraps each in `lab::rel_path_to(...)`.
- **`import std` and `import cstd` are package imports**, not files. They become
  `import "@std/build.lab"` in generated code — the leading `@` is the package marker.
- **`option` keys are validated at parse time.** An unknown key or a value of the wrong kind
  is a parse error (`parser/statements/OptionStmt.cpp:102-123`); see the
  [option table](#option-value-keys).
- **Options must stay in sync** across `ModuleOptionRegistry`, `LabModuleOptions.h`, and
  `lang/libs/lab/src/lab.ch` because the struct crosses CBI.
- **Remote imports are not part of the local dependency list** and are resolved in a separate
  pass (`process_remote_imports`); they don't appear in the `deps` array of the generated
  `build()`.
- **`version` is not part of the remote dedup key** — different versions of one repo merge
  and are resolved by `ConflictResolutionStrategy` (default `PreferNewerVersion`). Non-semver
  versions raise an error. See [`module_import`](../module_import/SKILL.md).
- **Ordering of remote `if` blocks matters for orphan branches** — put the more specific
  condition (`if windows and arm`) in its own statement; each is evaluated independently at
  runtime.
- **`link path` does not accept a bare `path` as a library name.** `link path "./dir"` must be
  quoted and is treated as a search directory; `link "mylib"` is the named library form.
- **`interface` names are compiler-interface identifiers**, not types you import. They must
  match an `@compiler.interface` declared in a library (e.g. `BuildContext` in `lab`).
- **No comments/CDATA restrictions** — `//` line comments work throughout the file
  (the `mbedtls` sample uses them).

## Related Skills

- [`module_import`](../module_import/SKILL.md) — how imports are resolved, the dependency
  graph, remote git download, version conflict resolution, and the per-module records.
- [`build_system`](../build_system/SKILL.md) — `chemical.mod` → `build.lab` → TinyCC JIT →
  jobs, the `BuildContext` API, and `ASTProcessor`.
- [`cbi_plugin_api`](../cbi_plugin_api/SKILL.md) — compiler plugins and `@compiler.interface`.
- [`compiler_bindings`](../compiler_bindings/SKILL.md) — CBI, through which `BuildContext` is
  exposed to `build.lab`.
- [`chemical_source`](../chemical_source/SKILL.md) — language syntax for the `.ch` sources a
  module compiles.
