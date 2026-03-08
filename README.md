<div align="center" style="display:grid;place-items:center;">
<p>
    <a href="https://chemical.qinetik.org/" target="_blank"><img height="220" src="https://raw.githubusercontent.com/chemicallang/chemical/main/lang/assets/Logo.svg?sanitize=true" alt="Chemical logo"></a>
</p>
<h1>The Chemical Programming Language</h1>

[Website](https://chemicallang.com)
| [Docs](https://docs.chemicallang.com)
| [Changelog](https://github.com/chemicallang/chemical/releases)
| [Contributing & compiler design](https://github.com/chemicallang/chemical/blob/main/CONTRIBUTING.md)

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

---

## ⚙️ Installation

1. Download & Extract the latest ZIP from the [Releases](https://github.com/chemicallang/chemical/releases) page.
2. Inside the folder, run `./chemical --configure` or `./chemical.exe --configure`
3. Verify by running `chemical -v`

---

## 🌟 Features

- **Easy to Learn:** Master in under a week.
- **Great IDE Support:** LSP with syntax highlighting, completions, diagnostics, and more.
- **Compile-Time Evaluation:** Powerful `comptime` features.
- **Low Memory Footprint:** Tiny executables, no garbage collector.
- **Multiple Backends:** LLVM & C (both fully functional).
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
| Basic LSP                     | ✅              |
| Standard Library & Docs       | 🔄 In Progress |
| Embedded Languages            | 🔄 In Progress |
| Advanced LSP Support          | 🔄 Planned     |
| Memory Management & Safety    | 🔄 Partial     |
| Advanced Build System Support | 🔄 Planned     |
| Comptime Features             | 🔄 Planned     |
| Compiler Plugins              | 🔄 Planned     |
| Mobile & Web Support          | 🔄 Planned     |

---

## 📚 Language Features

These features should give you an idea about features I have worked on

- C-like syntax with structs & namespaces
- Arrays, enums, unions
- Native lambdas (with capture)
- Macros & annotations
- Implicit & explicit casting
- Extension functions (like Kotlin)
- Raw pointers & memory control
- Full constructors & destructors
- Explicit copying (`.copy()` required)
- Comptime support & generics
- Overloading, variants, type aliases
- Trait & impl (Rust-like)
- Name mangling & conflict detection

---

## 🛠️ Build (From Source)

For detailed instructions on building the Chemical Compiler for various platforms, IDEs, and scenarios, please see the [Building Chemical](lang/docs/build/BUILDING.md) guide.

### Requirements

- 8–16 GB RAM
- C++ toolchain (for LSP)
- LLVM (for compiler)
- CLion or other IDE

#### LSP

1. For LSP: clone `chemical-vscode`.
2. There's a run configuration for compiling and launching extension
3. Build and Launch the LSP server before launching the extension, The extension detects running lsp executable at port automatically

*Open an issue for any build errors.*

---

# 🎯 Vision & Design Goals

## Compiler plugins and language extensions — Scalability

Chemical exists to let you embed complex, domain-specific syntax into the language via compiler plugins. Extensions are first-class: add new syntax and behaviors without touching the core. Tooling (syntax highlighting, editor parsing) must continue to work for extended syntax. Scalability and extensibility are part of Chemical’s DNA.

## Comprehensive features

Real projects frequently need features a language didn’t originally include. Chemical aims to provide the capabilities you actually need while avoiding “syntax pollution.” Powerful, composable abstractions for library authors and power users come with sensible defaults and a gentle learning curve for beginners. The goal: lots of capability, minimal surprise.

## Memory safety — without the cost of simplicity

Long-term, Chemical will promote memory safety while keeping the language simple to use. Safety is a priority but not yet fully enforced — the initial focus is on core functionality and tooling. Once the core is stable, compiler-enforced safety checks will be added progressively; the beta compiler will enforce baseline safety guarantees.

---

## 🤝 Contributing

We welcome all contributions! See [CONTRIBUTING.md](https://github.com/chemicallang/chemical/blob/main/CONTRIBUTING.md) for guidelines.

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