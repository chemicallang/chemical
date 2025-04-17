<div align="center" style="display:grid;place-items:center;">
<p>
    <a href="https://chemical.qinetik.org/" target="_blank"><img height="220" src="https://raw.githubusercontent.com/chemicallang/chemical/main/lang/assets/Logo.svg?sanitize=true" alt="Chemical logo"></a>
</p>
<h1>The Chemical Programming Language</h1>

[Website](https://chemical.qinetik.org)
| [Docs](https://github.com/chemicallang/chemical/blob/main/lang/docs/README.md)
| [Changelog](https://github.com/chemicallang/chemical/blob/main/lang/docs/CHANGELOG.md)
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

Key design goals:
- Prioritize safety, performance, and IDE integration.
- Reuse and extend existing concepts rather than reinventing.
- Allow language plugins to aid code generation

---

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

1. Download the latest ZIP from the [Releases](https://github.com/qinetik/chemical/releases) page.
2. Extract and add the folder to your `PATH`.
3. Verify:
   ```bash
   chemical -v
   ```
4. Run the initial setup:
   ```bash
   chemical configure
   ```

---

## 🌟 Features

- **Easy to Learn:** Master in under a week.
- **Great IDE Support:** LSP with syntax highlighting, completions, diagnostics, and more.
- **Compile-Time Evaluation:** Powerful `comptime` features.
- **Low Memory Footprint:** Tiny executables, no garbage collector.
- **Multiple Backends:** LLVM & C (fully functional).
- **C Interop:** Import system headers, translate between C & Chemical.
- **Flexible Build System:** Custom DSL for builds and modules.

---

## 📊 Progress & Roadmap

| Component                  | Status         |
|----------------------------|----------------|
| Lexer, Parser, Sym Res     | ✅              |
| Basic LSP                  | ✅              |
| Native Codegen (LLVM)      | ✅              |
| C Translation & TCC JIT    | ✅              |
| Custom Build System        | ✅              |
| Standard Library & Docs    | 🔄 In Progress |
| Embed Langs (HTML & CSS)   | 🔄 In Progress |
| Advanced LSP Support       | 🔄 Planned     |
| Memory Management & Safety | 🔄 Partial     |
| Comptime Features          | 🔄 Planned     |
| Multi-threaded Compiler    | 🔄 Planned     |
| Compiler Plugins           | 🔄 Planned     |
| Mobile & Web Support       | 🔄 Planned     |

*More details in the [CHANGELOG](https://github.com/chemicallang/chemical/blob/main/lang/docs/CHANGELOG.md).*

---

## 📚 Language Features

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
- C++ toolchain & Boost (for LSP)
- LLVM (for compiler)
- CLion or other IDE

### Getting Started

1. Clone `chemical-bootstrap` in the organization.
2. Inside it, clone this repo
3. Run build scripts (`build.bat` / `./build`).
4. For LSP: install Boost and clone `chemical-vscode`.
5. Open in your IDE and enjoy!

*Open an issue for any build errors.*

---

## 🎯 Vision & Design Goals

- **No traditional bootstrap:** leverage existing compilers.
- **Sensible simplicity:** one way to do things.
- **Extensible via plugins:** easy compiler extensions.
- **Git-based modules:** no central package repository. build system baked in

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