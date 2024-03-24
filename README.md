## Chemical Programming Language

Chemical is an innovative, performant, typesafe user-friendly programming language with a low memory footprint.

Chemical Programming Language brings language tooling by default & is customisable by the end developer and not a library author.
It focuses on reusing and adding more to what's already built.
It allows you to rethink how programming languages mix and share concepts to create solutions that are tailored to syntax preferred by the project you are working on !

### Syntax

It's similar to golang, typescript & c++ but there's a lot that cannot be explained about syntax here.

```golang
var x : int = 5;
for(var i = 0; i < 5; i++){
    var arr = []int(5); // array of five integer elements, uninitialized
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
// in the future, we will support switch statements, pointers
```

### Progress

A lot of unexplained things in this list, Trust me when I say,
very innovative things are planned.

- [x] Lexer
- [x] Parser
- [x] LSP (will add more features later)
- [ ] Interpreter (mostly done)
- [ ] Native Codegen (mostly done)
- [ ] CIPs
  - [ ] Syntax Modifier Plugins (SMP)
  - [ ] AST Transformation Plugins (ATP)
  - [ ] Auto Bootstrapping
- [ ] Documentation
- [ ] Experiment & Research
  - [ ] Multi language linking
  - [ ] Multiplatform syntax support
  - [ ] Embeddable Foreign Language Syntax
  - [ ] Data Oriented Code Generation API
- [ ] Language Growth & Stability
- [ ] Workspace Tooling

### Features

These features are sorted by priority, the features on top are what we will work to provide.

- Compile Time Evaluation
- Low memory footprint
- No Garbage Collection
- Compile time strict memory checks
- Tiny executables
- Everything Included Kit
- Native language (being worked on)
- Use as a code generator
- Use as an embedder language
- Use as a scripting language

### Build

#### Requirements

These requirements are for people who've never set up a C++ / Boost / LLVM Project

 - A PC with at least 8 - 16 GB of RAM
 - CLion IDE (I use CLion)
 - Toolchain (Visual Studio / Other)
 - LLVM (Optional -> only if you are working on compiler)
 - Patience with build errors (open an issues for fast response)

#### Instructions

The project generates three executables (at the moment)

 - Compiler (source code to executables) (requires llvm)
 - Interpreter (interprets the source code)
 - LSP Server (for IDE experience) (requires boost)

The easiest to setup and work on is interpreter because it has no dependencies.

#### To work on `Interpreter`
- clone the repo
  - open in CLion
  - mostly done, if you fail, open an issue

#### To work on `LSP Server`
- Please install Boost
  - open an issue, if you fail, I'll respond fast

#### To work on `Compiler`
- clone a repo present in this organization named `chemical-bootstrap`
- clone this repo inside `chemical-bootstrap` (folder chemical)
- build llvm, `chemical-bootstrap` has build scripts (build.bat or build)
- make sure it works, open an issue if it doesn't !

### Contibuting

Chemical Programming Language is an open source work, It also allows you to completely customize it.
It requires a lot of contributions to support the large number of features that need to be supported,
so we not just welcome contributions but encourage everybody to contribute. So we can bring our vision to life.