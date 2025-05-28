<div align="center" style="display:grid;place-items:center;">
<p>
    <a href="https://chemical.qinetik.org/" target="_blank"><img height="220" src="https://raw.githubusercontent.com/chemicallang/chemical/main/lang/assets/Logo.svg?sanitize=true" alt="Chemical logo"></a>
</p>
<h1>The Chemical Programming Language</h1>

[Website](https://chemical.qinetik.org)
| [Docs](https://github.com/chemicallang/chemical/blob/main/lang/docs/README.md)
| [Changelog](https://github.com/chemicallang/chemical/releases)
| [Speed](https://chemical.qinetik.org/speed)
| [Contributing & compiler design](https://github.com/chemicallang/chemical/blob/main/lang/docs/CONTRIBUTING.md)

</div>

<div align="center" style="display:grid;place-items:center;">

[![Sponsor][SponsorBadge]][SponsorUrl]
[![Patreon][PatreonBadge]][PatreonUrl]
[![Discord][DiscordBadge]][DiscordUrl]
[![X][XBadge]][XUrl]

</div>

## 🚀 Overview

Chemical is an innovative, performant, type-safe, and user-friendly programming language with a low memory footprint. It comes with first-class tooling out of the box, all customizable by developers.

---

> [!IMPORTANT]  
> Chemical is in pre-alpha state, expect breaking changes and do not use in production.

## 💻 Syntax

Resembles Go, TypeScript, and C++, with powerful extensions:

```chemical
var x: int = 5

for (var i = 0; i < 5; i++) {
  var arr: int[5]    // uninitialized array of 5 ints
  if (i == 3) x += 2
  if (i == 6) break
  x++
}

func add(a: int, b: int): int {
  return a + b
}

// Semicolons are optional
// Supports structs, interfaces, implementations, lambdas, switch, pointers
```

---

## ⚙️ Installation

1. Download & Extract the latest ZIP from the [Releases](https://github.com/chemicallang/chemical/releases) page.
2. Inside the folder, run `./chemical configure` or `./chemical.exe configure`
3. Verify by running `chemical -v`

---

## 🌟 Features

- **Easy to Learn:** Master in under a week.
- **Great IDE Support:** LSP with syntax highlighting, completions, diagnostics, and more.
- **Compile-Time Evaluation:** Powerful `comptime` features.
- **Low Memory Footprint:** Tiny executables, no garbage collector.
- **Multiple Backends:** LLVM & C (fully functional).
- **C Interop:** Translate between C & Chemical.
- **Flexible Build System:** Custom DSL for builds and modules.

---

## 📊 Progress & Roadmap

| Component                     | Status         |
|-------------------------------|----------------|
| Lexer, Parser, Sym Res        | ✅              |
| Native Codegen (LLVM)         | ✅              |
| C Translation & TCC JIT       | ✅              |
| Basic Build System            | ✅              |
| Basic Multi-threading         | ✅              |
| Basic LSP                     | 🔄 In Progress |
| Standard Library & Docs       | 🔄 In Progress |
| Embedded Languages            | 🔄 In Progress |
| Advanced LSP Support          | 🔄 Planned     |
| Memory Management & Safety    | 🔄 Partial     |
| Advanced Build System Support | 🔄 Planned     |
| Comptime Features             | 🔄 Planned     |
| Advanced Multi-threading      | 🔄 Planned     |
| Compiler Plugins              | 🔄 Planned     |
| Mobile & Web Support          | 🔄 Planned     |

---

## 📚 Language Features

These features should give you an idea about features we have worked on

- C-like syntax with structs & namespaces
- Arrays, enums, unions
- Native lambdas (with capture)
- Macros & annotations
- Implicit & explicit casting
- Extension functions
- Raw pointers & memory control
- Full constructors & destructors
- Explicit copying (`.copy()` required)
- Comptime support & generics
- Overloading, variants, type aliases
- Trait & impl (Rust-like)
- Name mangling & conflict detection

---

## 🛠️ Build (From Source)

### Requirements

- 8–16 GB RAM
- C++ toolchain (for LSP)
- LLVM (for compiler)
- CLion or other IDE

### Getting Started

1. Clone `chemical-bootstrap` in the organization (contains LLVM/CLANG)
2. Run build scripts (`./build.bat` / `./build`).
3. Inside it, clone this repo
4. Open this repo in your IDE and enjoy!

#### Building the TCC based compiler

TCC based compiler build requires some tcc files to be present at runtime, this process will be automated someday

1. If you use CLion, know that output dir would be `cmake-build-debug` (next to the TCCCompiler / ChemicalLSP)
2. do not touch / copy `$this_repo/lib/libtcc/include` (directory)
3. put contents of either `$this_repo/lib/libtcc/win-x64` or `$this_repo/lib/libtcc/lin-x64` into `$output_dir/packages/tcc`
4. make sure to extract the `package.zip` into `$output_dir/packages/tcc` directory
5. copy the `libtcc.dll` or `libtcc.so` into the `$output_dir/`, your final structure should be

```
directory: $output_dir/packages/tcc/include
directory: $output_dir/packages/tcc/lib
file:      $output_dir/libtcc.dll
```

#### LSP

1. For LSP: clone `chemical-vscode`.
2. There's a run configuration for compiling and launching extension
3. Build and Launch the LSP server before launching the extension, The extension detects running lsp executable at port automatically

*Open an issue for any build errors.*

---

## 🎯 Vision & Design Goals

- **No traditional bootstrap:** leverage existing compilers

   We won't be doing a traditional bootstrap where we write the compiler in our own language, We'll use
   C/C++ to write the compiler and instead of bootstrap, we'll provide features like binding with the compiler
   with the use of our custom build system.

- **Sensible simplicity:** one way to do things.

   We don't want to make the syntax complex, the language should feel very easy to a moderately experienced programmer
   However too much simplicity (like Python) is also not desired.

- **Memory Safety:** without the cost of simplicity

   This is more of a long term goal, to promote memory safety, We want to be as safe as possible without sacrificing simplicity.

- **Git-based modules:** no central package repository. build system baked in

   Compiler would automatically provide support for package management without having to download external package management
   libraries / plugins. Compiler would provide support for it in build files and its command line by using git to manage
   modules.

---

## 🤝 Contributing

We welcome all contributions! See [CONTRIBUTING.md](https://github.com/chemicallang/chemical/blob/main/lang/docs/CONTRIBUTING.md) for guidelines.

---

## 📄 License

Chemical compiler is MIT-licensed and will remain open source. Use it freely—credit is appreciated but not required.

---

<!-- Badges -->
[WorkflowBadge]: https://github.com/vlang/v/workflows/CI/badge.svg
[DiscordBadge]: https://img.shields.io/discord/1206227290359337062?label=Discord&logo=discord&logoColor=white
[PatreonBadge]: https://img.shields.io/endpoint.svg?url=https%3A%2F%2Fshieldsio-patreon.vercel.app%2Fapi%3Fusername%3Dwakaztahir%26type%3Dpatrons&style=flat
[SponsorBadge]: https://img.shields.io/github/sponsors/wakaztahir?style=flat&logo=github&logoColor=white
[XBadge]: https://img.shields.io/badge/follow-%40qinetikorg-1DA1F2?logo=x&style=flat&logoColor=white

[WorkflowUrl]: https://github.com/chemicallang/chemical/commits/main
[DiscordUrl]: https://discord.gg/uYU4SV9avu
[PatreonUrl]: https://patreon.com/wakaztahir
[SponsorUrl]: https://github.com/sponsors/wakaztahir
[XUrl]: https://x.com/qinetikorg