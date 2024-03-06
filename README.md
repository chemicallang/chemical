## Chemical Programming Language

Chemical is an innovative, performant, typesafe user-friendly programming language with a low memory footprint.

Chemical Programming Language brings language tooling by default & is customisable by the end developer and not a library author.
It focuses on reusing and adding more to what's already built.
It allows you to rethink how programming languages mix and share concepts to create solutions that are tailored to syntax preferred by the project you are working on !

### Syntax

We are aiming for typescript like syntax for this native language.

```typescript
    var x : int = 5;
for(var i = 0; i < 5; i++){
    // switch statements are similar to c++
    if(i == 3) {
        x += 2;
    }
    if(i == 6) break;
    x++;
}
// while and do while loops are also similar
// a function, requires types
function add(a : int, b : int) : int {
    return a + b;
}
// supports : struct, interface, implementation
// in the future, we will support switch statements, pointers
```

### Progress

- [x] Lexer
- [x] Parser
- [x] LSP (will add more features later)
- [ ] Interpreter (mostly done)
- [ ] Native Codegen
- [ ] CIPs
  - [ ] Syntax Modifier Plugins (SMP)
  - [ ] AST Transformation Plugins (ATP)
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
- Everything Included Kit
- Use as an interpreted / native language (being worked on)
- Use as a template engine (we may skip this)
- Use as a code generator
- Use as an embedder language
- Use as a scripting language (we may skip this)

### Contibuting

Chemical Programming Language is an open source work, It also allows you to completely customize it.
It requires a lot of contributions to support the large number of features that need to be supported,
so we not just welcome contributions but encourage everybody to contribute. So we can bring our vision to life.