## Command Line

Chemical's Compiler is the easiest to use when it comes to command line

Let's see the commands in action

#### Compile a single file to executable
```shell
compiler.exe file.ch -o file.exe
```

#### Compile a single file to object
```shell
compiler.exe file.ch -o file.o
```

#### Translate a single file into C
```shell
compiler.exe file.ch -o file.c
```

#### Translate C into Chemical
Compiled based on Tiny CC doesn't support this operation

```shell
compiler.exe file.c -o file.ch
```

#### Compile a single file to llvm ir
```shell
compiler.exe file.ch -o file.ll
```

As you can see we only changed the extension, That's all it takes

Apart from extensions above, .bc (llvm bitcode), .s (assembly) are supported, but depends on whether we built the compiler
with these features enabled. Currently, we don't. But it's likely we may do it in the future

Let's see some other options

- `--verify`, `-verify`
  - do not compile the code, only verify the source code
- `--benchmark`, `-bm`
  - benchmark lexing or compilation process
- `--print-ig`, `-pr-ig`
  - prints import graph, which is constructed before compiling
- `--print-ast`, `-pr-ast`
  - AST is printed to console, a representation of it, for quick debugging
- `--print-ir`, `-pr-ir`
  - LLVM IR is printed to console, for quick debugging
- `--print-cst`, `-pr-cst`
  - CST is printed to console, for quick debugging
- `--jit`, `-jit`
  - Just in time compilation is enabled (Tiny CC only)
- `--tcc`, `-tcc`
  - Force use Tiny CC backend for code generation


### Translate To C

- `--no-cbi`
  - Disables CBI for translation, all the nodes will be considered for translation

- `--cpp-like`
  - When given output will be more like c++, for example bool instead of _Bool
  - This is mostly used by us to generate CBI's