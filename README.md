<div align="center" style="display:grid;place-items:center;">
<p>
    <a href="https://chemical.qinetik.org/" target="_blank"><img height="220" src="https://raw.githubusercontent.com/Qinetik/chemical/main/lang/assets/Logo.svg?sanitize=true" alt="Chemical logo"></a>
</p>
<h1>The Chemical Programming Language</h1>

[Website](https://chemical.qinetik.org)
| [Docs](https://github.com/Qinetik/chemical/blob/main/lang/docs/README.md)
| [Changelog](https://github.com/Qinetik/chemical/blob/main/lang/docs/CHANGELOG.md)
| [Speed](https://chemical.qinetik.org/speed)
| [Contributing & compiler design](https://github.com/Qinetik/chemical/blob/main/lang/docs/CONTRIBUTING.md)

</div>

<div align="center" style="display:grid;place-items:center;">

[![Sponsor][SponsorBadge]][SponsorUrl]
[![Patreon][PatreonBadge]][PatreonUrl]
[![Discord][DiscordBadge]][DiscordUrl]
[![X][XBadge]][XUrl]

</div>
Chemical is an innovative, performant, typesafe user-friendly programming language with a low memory footprint.

Chemical Programming Language brings language tooling by default & is customisable by the end developer and not a library author.
It focuses on reusing and adding more to what's already built.
It allows you to rethink how programming languages mix and share concepts to create solutions that are tailored to syntax preferred by the project you are working on !

## Syntax

It's similar to golang, typescript & c++ but there's a lot that cannot be explained about syntax here.

```golang
import "file.ch"
var x : int = 5;
for(var i = 0; i < 5; i++){
    var arr = {}int(5); // array of five integer elements, uninitialized
    if(i == 3) {
        x += 2;
    }
    if(i == 6) break;
    x++;
}
// while and do while loops are also similar
// a function, requires types, primitive values are pass by value
func add(a : int, b : int) : int {
    return a + b;
}
// supports : struct, interface, implementation
// semicolons are optional
// supports lambdas, switch statements & pointers
```

## Features

- So easy that you can learn it in a week !
- Great IDE support
- Compile Time Evaluation
- Low memory footprint
- LLVM Backend, C Backend (both fully functional)
- Translate C to Chemical
- Translate Chemical to C
- Import C System Headers
- No Garbage Collection
- Compile time memory checks
- Tiny executables
- Native language
- Use as an embedder language
- Custom Flexible Build System
  - with our own language

## Progress

A lot of unexplained things in this list, Trust me when I say,
very innovative things are planned.

- [x] Lexer
- [x] Parser
- [x] LSP
  - [x] Syntax Highlighting
  - [x] Completion items
  - [x] Diagnostics
  - [x] Imported Files support
  - [x] Resolve symbol support
  - [ ] Improved diagnostics support
  - [x] Semantic highlighting
  - [x] Member access completion items
  - [x] Hover doc support
  - [ ] Inlay Hints
  - [ ] Find usages support
  - [ ] Refactor rename
  - [ ] Formatting
  - [ ] Codelens support
- [x] Native Codegen (LLVM)
- [x] Necessary C Translation (can import system headers)
- [x] Translate to C (all tests passing)
- [x] Just In Time Compilation using Tiny CC
- [x] Custom Build System
- [ ] Improved support for Comp Time
- [ ] Improved support for Typealiases
- [x] Memory Management
  - [x] C++ like constructors & destructors
  - [ ] Strict checks for undefined behaviour
  - [ ] Better API for Memory control
- [ ] Compile time Interpreted Plugins
  - [x] Compiler Binding Interface (CBI)
  - [ ] Syntax Modifier Plugins (SMP)
  - [ ] AST Transformation Plugins (ATP)
  - [ ] Annotation / Symbol Processing
- [x] Build System based on Chemical
  - [x] Compile single / multiple root file as modules
  - [x] Link multiple modules
  - [x] Work with both TCC and Clang based
  - [x] Translate Files to C (currently only TCC based)
  - [x] Generate llvm ir / assembly / bitcode / object files
  - [ ] Directory module automatic compilation
  - [ ] Can import other modules (implemented but untested)
  - [ ] Run executables after building them
  - [x] Can generate shared objects / dll
  - [ ] Can generate static libraries
  - [x] Can compile and link C module
  - [ ] Configure compile / link parameters
- [ ] Maintenance Phase
  - [ ] Eliminate bugs, Stabilize
  - [ ] Standard Library
  - [ ] Documentation
  - [ ] Support proper C Translation
  - [ ] Workspace Tooling
- [ ] Experiment & Research
  - [ ] Embeddable Foreign Language Syntax
  - [ ] Web Support
- [ ] Language Growth
  - [ ] Mobile : Android, iOS Support
  - [ ] More platforms


## Build

### Requirements

These requirements are for people who've never set up a C++ / Boost / LLVM Project

 - A PC with at least 8 - 16 GB of RAM
 - CLion IDE (I use CLion)
 - Toolchain (Visual Studio / Other)
 - LLVM (Optional -> only if you are working on compiler)
 - Patience with build errors (open an issues for fast response)

### Instructions

The project generates these executables (and maybe more)

 - Compiler (source code to executables) (requires llvm & clang)
 - TCC Compiler (a compiler based on Tiny CC compiler)
 - LSP Server (for IDE experience) (requires boost)

#### To work on `LSP Server`
- Please install Boost
  - open an issue, if you fail
- clone a repo inside this organization named `chemical-vscode`

#### To work on `Compiler`
- clone a repo present in this organization named `chemical-bootstrap`
- create a folder inside `chemical-bootstrap`, name it `chemical`
- clone this repo inside `chemical`
- build llvm, `chemical-bootstrap` has build scripts (build.bat or build)
- open an issue, if you fail

## Vision & Design goals

 - Chemical will never be bootstrapped. At least not in a traditional sense. 
 - Don't want to waste time writing features again and again. 
 - Want to keep it simple, Language plugins instead of syntactic sugar.
 - Won't provide multiple ways of doing things.
 - Will provide a very easy way to write compiler plugins.
 - No package manager, instead command line Git Interface with modules & shallow clones.

## Contributing

Chemical Programming Language is an open source work, It also allows you to completely customize it.
It requires a lot of contributions to support the large number of features that need to be supported,
so we not just welcome contributions but encourage everybody to contribute. So we can bring our vision to life.


## Licensing

Qinetik owns Chemical, Qinetik is a for-profit company. But we will always keep Chemical as an open source programming language.

Qinetik intends to make products and apps and intends to develop Chemical as an open source work with community effort to improve
app development process.

Chemical's Compiler is open source and MIT licensed and will always remain open source and MIT licensed, You can use Chemical's compiler however you like.
You don't even need to give credits. But if you do, we'd really appreciate it. 

[WorkflowBadge]: https://github.com/vlang/v/workflows/CI/badge.svg
[DiscordBadge]: https://img.shields.io/discord/1206227290359337062?label=Discord&logo=discord&logoColor=white
[PatreonBadge]: https://img.shields.io/endpoint.svg?url=https%3A%2F%2Fshieldsio-patreon.vercel.app%2Fapi%3Fusername%3Dwakaztahir%26type%3Dpatrons&style=flat
[SponsorBadge]: https://img.shields.io/github/sponsors/wakaztahir?style=flat&logo=github&logoColor=white
[XBadge]: https://img.shields.io/badge/follow-%40qinetikorg-1DA1F2?logo=x&style=flat&logoColor=white

[WorkflowUrl]: https://github.com/Qinetik/chemical/commits/main
[DiscordUrl]: https://discord.gg/uYU4SV9avu
[PatreonUrl]: https://patreon.com/wakaztahir
[SponsorUrl]: https://github.com/sponsors/wakaztahir
[XUrl]: https://x.com/qinetikorg