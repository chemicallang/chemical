# Chemical Cross-Compilation Architecture — Codebase Analysis Requirements

## 1. Purpose

We want to extend the Chemical compiler to support **cross-compilation** in a way that preserves our core design goals:

* Chemical should be able to compile for a target different from the host OS/architecture.
* Users should not need to install a separate C/C++ compiler, LLVM, linker, system SDK, or cross-compilation toolchain.
* Target-specific compiler components should be downloadable on demand.
* The base Chemical compiler should remain as small and self-contained as reasonably possible.
* We want both of our existing compilation paths to support cross-compilation:

  * the **TinyCC/C backend**
  * the **LLVM backend**
* We want to understand what changes are actually necessary in our existing codebase before implementing anything.

This document is an instruction to another AI that has access to and understands the Chemical codebase.

---

# 2. Existing Chemical Architecture

Chemical currently has two major compilation/code-generation paths.

## 2.1 C / TinyCC backend

Chemical can translate its code into C.

The generated C is intentionally very self-contained.

A particularly important property is:

> **Our generated C does not include system headers.**

We do not generate things such as:

```c
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
```

Instead, when necessary, the generated C declares the external functions/types/interfaces it needs itself.

This is intentional and important for cross-compilation.

The generated C can then be compiled using TinyCC.

TinyCC is attractive to us because:

* it is very small;
* it has a small API;
* it includes its own compiler/linker functionality;
* it does not require a large external compiler installation;
* it is relatively easy for us to build for different hosts/targets;
* it is much easier to distribute than a conventional GCC/Clang toolchain.

Our current idea is to build target-capable TinyCC libraries for the hosts we support and distribute them as downloadable backend components.

For example:

```text
Chemical on Windows x64
        |
        +---- TinyCC Windows-host backend
        |
        +---- generates Windows x64
```

but if the user requests:

```text
chemical build foo.chem --target linux-x64
```

Chemical should be able to obtain a suitable TinyCC backend and use it to compile the generated C into Linux x64 objects/executables.

The exact implementation details need to be determined by analyzing the current TinyCC integration.

---

# 3. Existing LLVM Architecture

Chemical also has an LLVM backend.

Chemical generates LLVM IR using LLVM APIs such as:

* `llvm::Module`
* `IRBuilder`
* LLVM IR/code-generation infrastructure
* LLVM optimization/code-generation components
* other LLVM APIs already present in the codebase.

Our current LLVM distribution is relatively large, approximately **60 MB**.

The current build includes:

* LLVM libraries
* all LLVM target backends
* Clang libraries
* LLD libraries
* other components required by our current implementation.

We obtained/maintain the LLVM build workflow based on the Zig / zig-bootstrap approach and currently retain this workflow.

The current implementation is therefore effectively:

```text
Chemical
   |
   +-- LLVM
   |    |
   |    +-- all targets
   |    +-- LLVM libraries
   |    +-- Clang libraries
   |    +-- LLD libraries
   |
   +-- generated LLVM IR
```

The current LLVM setup can invoke Clang/toolchain functionality where appropriate.

Our linker invocations are not yet perfect, and this needs to be investigated as part of the work.

---

# 4. Main Goal

We want Chemical to support commands conceptually like:

```bash
chemical enable target linux-x64
```

and:

```bash
chemical build main.chem --target linux-x64
```

The user may be running Chemical on Windows while compiling for Linux.

For example:

```text
HOST:
Windows x64

TARGET:
Linux x64
```

Chemical should be able to produce a Linux x64 executable without requiring the user to install Linux GCC/Clang/binutils/etc.

Likewise, eventually:

```text
Windows x64 → Linux x64
Windows x64 → Linux ARM64
Windows x64 → Windows ARM64
Linux x64   → Windows x64
Linux x64   → Linux ARM64
...
```

depending on which targets we officially support.

---

# 5. Desired Backend Architecture

The central idea is that **the host compiler should not need to contain every possible target**.

We want a base Chemical compiler and downloadable target-specific components.

Conceptually:

```text
                  Chemical
                     |
             target requested
                     |
          +----------+----------+
          |                     |
          v                     v
       TinyCC                  LLVM
          |                     |
          v                     v
    target backend        target backend
          |                     |
          v                     v
       object                object
          |                     |
          +----------+----------+
                     |
                  linker
                     |
                     v
                executable
```

The target backend should be loaded dynamically.

---

# 6. LLVM Target-Backend Concept

We are specifically considering separating the LLVM installation into:

## Base LLVM

Contains target-independent/common functionality such as:

* LLVM IR
* `llvm::Module`
* `IRBuilder`
* LLVM analysis
* LLVM transformation/optimization infrastructure
* common code-generation infrastructure
* MC/object infrastructure
* other LLVM functionality actually required by Chemical
* any other common libraries that are genuinely shared by all targets.

## Target LLVM backend

Contains architecture-specific code generation, for example:

```text
X86
AArch64
ARM
RISC-V
WebAssembly
...
```

The target backend would be dynamically loaded.

Conceptually:

```text
chemical_llvm_core.dll
        +
chemical_llvm_target.dll
```

For x86-64:

```text
chemical_llvm_core.dll
chemical_llvm_x86.dll
```

For AArch64:

```text
chemical_llvm_core.dll
chemical_llvm_aarch64.dll
```

The exact DLL/library structure is NOT predetermined. The codebase analysis should determine the best practical implementation.

---

# 7. Important LLVM Question

We need to determine whether the existing LLVM code can be cleanly divided into:

```text
COMMON LLVM
        +
TARGET-SPECIFIC LLVM
```

such that the base compiler does not contain all LLVM targets.

We want the analysis to inspect the actual codebase and determine:

1. Which LLVM libraries we currently use.
2. Which of those are target-independent.
3. Which are target-specific.
4. Which libraries are pulled in indirectly.
5. Which APIs initialize/register LLVM targets.
6. Whether our current code assumes that all LLVM targets are statically registered.
7. Whether we can dynamically load/register a target backend.
8. Whether our current build system can produce a common LLVM library plus separately loadable target libraries.
9. Whether LLVM's current CMake/build configuration supports the required separation cleanly.
10. Which pieces of Clang we actually need.
11. Which pieces of LLD we actually need.
12. Whether Clang can be removed from the core LLVM distribution.
13. Whether LLD should remain in the base compiler or become part of a target/toolchain package.
14. What the minimum set of LLVM components is for:

```text
LLVM IR → target object
```

15. What additional components are necessary for:

```text
target object → final executable
```

Do not assume the answer. Inspect the actual code.

---

# 8. LLVM IR Portability

One important assumption we want verified against the actual codebase:

Chemical's LLVM IR should be largely target-independent.

Conceptually:

```text
Chemical source
      |
      v
LLVM IR
      |
      +-------------------+
      |                   |
      v                   v
x86-64 backend       AArch64 backend
      |                   |
      v                   v
 x86-64 .o             ARM64 .o
```

We need the codebase analysis to identify anything in our LLVM generation that currently makes assumptions about:

* pointer size;
* ABI;
* alignment;
* data layout;
* calling conventions;
* target-specific intrinsics;
* target-specific LLVM instructions;
* CPU features;
* object format;
* platform-specific runtime behavior;
* Windows/Linux differences.

We want to know whether we can generate one target-independent LLVM module and then configure the appropriate target at the backend/code-generation stage.

If that is not currently true, identify exactly where the assumptions occur and what needs to change.

---

# 9. Object Files

We understand that object files are **not architecture-independent**.

For example:

```text
LLVM IR
    |
    +--> x86-64 backend --> x86-64 ELF/COFF object
    |
    +--> AArch64 backend --> AArch64 ELF object
```

The goal is therefore NOT to create a universal `.o`.

The desired boundary is:

```text
TARGET-INDEPENDENT:
Chemical AST
LLVM IR

TARGET-SPECIFIC:
machine code
object file
executable
```

The codebase analysis should verify that our current implementation can preserve this boundary.

---

# 10. Linking Is a Separate Problem

A major part of the analysis must distinguish:

```text
LLVM IR
   ↓
target object
```

from:

```text
target object
   ↓
final executable
```

Generating a target object does not automatically provide:

* startup objects;
* libc;
* compiler runtime;
* dynamic linker information;
* target system libraries;
* target SDK;
* linker;
* target-specific runtime libraries.

Our desired system should eventually provide whatever is necessary without requiring the user to install a separate cross-compilation toolchain.

Therefore analyze our current linker implementation carefully.

Determine:

1. Where we invoke Clang.
2. Where we invoke LLD.
3. Where we invoke the system linker.
4. Whether we depend on host system libraries.
5. Whether we depend on host object files.
6. Whether we depend on target object files.
7. Where CRT/startup objects come from.
8. Where libc/runtime libraries come from.
9. What sysroot assumptions exist.
10. Whether we can package the required target linker/runtime independently.

---

# 11. Clang

Our current LLVM build includes Clang.

We use Clang partly because its driver is convenient:

```text
clang
   |
   +-- understands target
   +-- invokes appropriate linker
   +-- handles various toolchain details
```

However, Chemical does **not** use Clang as its language frontend.

Chemical already produces:

```text
LLVM IR
```

Therefore we want the analysis to determine whether we actually need:

* Clang frontend;
* Clang AST;
* Clang parser;
* Clang semantic analysis;
* Clang tooling;
* Clang driver;
* or only a small subset of Clang functionality.

If Clang can be removed from the base compiler, determine how.

Do not remove it merely because it looks unnecessary; trace actual dependencies first.

---

# 12. LLD

We currently include LLD.

We want to determine whether LLD should:

### Option A

Remain in the base LLVM distribution.

### Option B

Be included in each target toolchain.

### Option C

Be split so that only required linker components are distributed.

### Option D

Be replaced by another mechanism.

The preferred architecture is likely something like:

```text
Chemical LLVM core
       |
       +-- target backend
       |
       +-- target linker/toolchain
```

but this must be validated against the current codebase.

---

# 13. Target Packages

We envision something conceptually like:

```text
~/.chemical/
    targets/
        linux-x64/
            llvm-target.dll
            linker
            runtime/
            sysroot/

        linux-arm64/
            llvm-target.dll
            linker
            runtime/
            sysroot/

        windows-x64/
            llvm-target.dll
            linker
            runtime/
            sysroot/
```

However, this is only a conceptual design.

The codebase analysis should recommend the actual packaging structure.

In particular, determine whether the following should be separated:

```text
architecture backend
target operating-system environment
linker
runtime
sysroot
```

For example, an LLVM x86-64 code generator may be usable for:

```text
x86-64 Linux
x86-64 Windows
x86-64 macOS
```

while the linker/runtime/sysroot differs.

Do not duplicate architecture-specific LLVM code unnecessarily.

---

# 14. Dynamic Loading

We want target-specific LLVM functionality to be dynamically loaded.

Potentially:

```text
chemical_llvm_core.dll
chemical_llvm_x86.dll
```

or:

```text
chemical_llvm_core.dll
chemical_llvm_target.dll
```

or another structure.

The exact ABI needs to be designed.

Analyze:

1. Which LLVM APIs are currently used directly by Chemical.
2. Which APIs would cross the DLL boundary.
3. Whether passing LLVM C++ objects such as `llvm::Module*` across DLL boundaries is safe with our current compiler/build configuration.
4. Whether we should expose a C ABI wrapper.
5. Whether LLVM's own C API is sufficient.
6. Whether the target DLL should receive an `llvm::Module`.
7. Whether the target DLL should instead receive serialized LLVM bitcode/IR.
8. Whether LLVM's existing target registration mechanisms can be isolated behind the DLL.
9. Whether static linking of common LLVM code into both DLLs would create ODR/ABI problems.
10. Whether LLVM's build settings must be identical across core and target DLLs.

This is particularly important.

We do NOT want to create a DLL architecture that is technically possible but unstable because LLVM C++ objects cross independently built DLL boundaries incorrectly.

---

# 15. TinyCC Backend Analysis

Perform the same analysis for TinyCC.

Determine:

1. How Chemical currently embeds/links TinyCC.
2. Which TinyCC APIs are used.
3. Which parts of TinyCC are host-specific.
4. Which parts are target-specific.
5. Whether one host-native TinyCC DLL can emit different target architectures.
6. What needs to be rebuilt for each target.
7. Whether the target's object formats are built into TinyCC.
8. Whether TinyCC's internal linker needs target-specific objects/libraries.
9. Which startup/runtime/system libraries are required.
10. Whether our generated C declarations are sufficient for cross compilation.
11. Whether our generated C contains any hidden host assumptions.
12. Whether our current TinyCC integration can dynamically load a target-specific TinyCC backend.

Our goal is to exploit TinyCC's small API and small build footprint.

---

# 16. TinyCC Desired Architecture

Conceptually:

```text
Chemical
    |
    v
generated C
    |
    v
host-native TinyCC backend
    |
    v
target object / executable
```

We want to distribute prebuilt TinyCC backend combinations.

For example:

```text
host = Windows x64
target = Linux x64
```

would require a Windows-native library capable of generating Linux x64 output.

We are considering storing these in GitHub Releases.

The target matrix may eventually be large, but TinyCC is small enough that this is acceptable.

We expect the total size of all TinyCC backend releases to remain manageable.

Verify this assumption and identify any hidden complications.

---

# 17. Cross Compilation Command Semantics

We want commands conceptually like:

```bash
chemical enable target linux-x64
```

and:

```bash
chemical build main.chem --target linux-x64
```

Potentially:

```bash
chemical targets list
chemical targets install linux-x64
chemical targets remove linux-x64
chemical targets update linux-x64
```

The codebase analysis should identify where target triples/configuration should live.

Determine:

* target parsing;
* host detection;
* target validation;
* backend selection;
* backend download;
* backend cache;
* dynamic loading;
* linker selection;
* runtime selection;
* output format;
* target-specific compiler flags.

Do not implement these yet unless explicitly requested. First provide an architectural/codebase analysis.

---

# 18. Backend Downloading

The intended UX is:

```bash
chemical build foo.chem --target linux-x64
```

If the backend is already installed:

```text
use cached backend
```

If not:

```text
target backend not installed
        ↓
download official backend
        ↓
verify it
        ↓
install/cache it
        ↓
compile
```

The analysis should identify where this functionality should be integrated into the existing compiler.

Consider:

* versioning;
* compiler version compatibility;
* LLVM version compatibility;
* backend ABI compatibility;
* host architecture;
* target architecture;
* target OS;
* package hashes;
* corrupted downloads;
* backend upgrades/downgrades.

Security is important: do not blindly execute arbitrary downloaded DLLs/binaries.

---

# 19. Important Host/Target Distinction

Be extremely careful to distinguish:

```text
HOST
```

from:

```text
TARGET
```

For example:

```text
Host:
Windows x64

Target:
Linux x64
```

The dynamically loaded backend itself must normally be executable by the **host**.

Therefore a Windows Chemical process cannot simply load a Linux `.so`.

The downloadable LLVM/TinyCC component must be built for the host while being capable of generating code for the target.

For example:

```text
Windows x64 Chemical
        |
        +-- Windows x64 DLL
        |      |
        |      └── generates Linux x64 code
        |
        v
Linux x64 executable
```

The analysis must account for this distinction throughout the implementation.

---

# 20. Current Build System

We currently have a substantial LLVM build workflow based on:

```text
Zig / zig-bootstrap
```

It currently builds LLVM/Clang/LLD and all targets.

Analyze the existing build scripts/workflow rather than proposing an unrelated LLVM build system.

Determine:

1. Where all targets are currently enabled.
2. How target libraries are selected.
3. Whether `LLVM_TARGETS_TO_BUILD` or equivalent configuration is currently used.
4. Whether we can build only one target.
5. Whether LLVM can be built as shared libraries.
6. Whether the current build can produce a common core plus target-specific library.
7. Whether the current Zig-bootstrap setup makes this easier or harder.
8. What changes are required to create the proposed packages.

---

# 21. What We Want From This Analysis

Do NOT immediately implement the feature.

First produce a detailed codebase-driven report containing:

## A. Current architecture

Explain exactly how Chemical currently uses:

* TinyCC;
* LLVM;
* Clang;
* LLD;
* generated C;
* LLVM IR;
* object files;
* linking.

Reference actual files/classes/functions/build scripts.

## B. Cross-compilation feasibility

Determine whether the desired architecture is technically feasible with the current codebase.

Separate:

```text
already works
minor changes
major changes
unknown / requires experiment
```

## C. Required changes

Give a concrete list of changes required for:

### TinyCC

```text
host → target
```

### LLVM

```text
LLVM IR → target object
```

### Linking

```text
target object → target executable
```

### Backend management

```text
enable/download/cache/load backend
```

## D. LLVM dependency graph

Produce a useful dependency breakdown:

```text
COMMON
├── ...
├── ...
└── ...

X86 TARGET
├── ...
└── ...

AARCH64 TARGET
├── ...
└── ...

CLANG
├── ...
└── ...

LLD
├── ...
└── ...
```

Identify which libraries can be removed from the base compiler.

## E. Recommended final architecture

Recommend the leanest architecture that still provides reliable cross compilation.

We want a concrete proposal such as:

```text
chemical
chemical-llvm-core
chemical-llvm-x86
chemical-llvm-aarch64
chemical-linker-linux-x64
...
```

but derive the actual names/structure from the codebase.

## F. Implementation phases

Give a staged implementation plan.

Prefer:

```text
Phase 1:
make cross compilation work with current 60 MB LLVM

Phase 2:
separate target-specific LLVM code

Phase 3:
remove unnecessary Clang components

Phase 4:
separate/package LLD and target runtime

Phase 5:
backend manager/download/cache
```

if that is appropriate.

Do not assume these phases are correct; modify them if codebase analysis shows a better approach.

---

# 22. Critical Constraints

Do not lose sight of these goals.

### Goal 1 — Cross compilation

A Windows user should be able to produce a Linux executable without installing a Linux compiler.

Example:

```bash
chemical build hello.chem --target linux-x64
```

### Goal 2 — Self-contained compiler

Chemical should not depend on:

```text
system GCC
system Clang
system LLVM
system LLD
```

being installed.

### Goal 3 — Small base installation

The base Chemical compiler should not contain every target backend if we can avoid it.

### Goal 4 — Download targets on demand

Users should only need to download targets they actually use.

### Goal 5 — Reuse LLVM IR

LLVM IR should remain the common intermediate representation before target-specific machine code generation.

### Goal 6 — TinyCC remains lightweight

TinyCC should remain a lightweight alternative to LLVM.

### Goal 7 — Do not unnecessarily duplicate architecture backends

An x86-64 LLVM backend should ideally be reusable for multiple x86-64 operating-system targets where appropriate.

### Goal 8 — Reliable final linking

Producing `.o` files is not sufficient.

We ultimately need:

```text
Chemical source
    ↓
target object
    ↓
target linker/runtime
    ↓
working target executable
```

### Goal 9 — No hidden host dependencies

Cross compilation must not accidentally use:

```text
host libc
host startup objects
host linker
host headers
host libraries
```

when producing a target executable.

### Goal 10 — Maintainability

Do not introduce an architecture that requires us to maintain a heavily modified LLVM fork unless absolutely necessary.

Prefer upstream LLVM mechanisms and build configuration wherever possible.

---

# 23. Questions That Must Be Answered Explicitly

At the end of the analysis, answer these questions directly:

1. **Can our current LLVM IR generation be reused for multiple targets?**
2. **Where does our current code assume the host target?**
3. **What exactly is required to turn our LLVM IR into a Linux x64 object?**
4. **What exact LLVM libraries are required for that?**
5. **Which of those libraries are target-specific?**
6. **Can target-specific LLVM code be moved into dynamically loaded DLLs?**
7. **Can our current LLVM C++ objects safely cross that DLL boundary?**
8. **Should we use a C ABI wrapper?**
9. **Can Clang be removed from the base LLVM distribution?**
10. **What part of Clang do we actually use?**
11. **Can LLD be separated from the base LLVM distribution?**
12. **What is required to link a Linux executable without a Linux toolchain installed?**
13. **Can we package the required target runtime/sysroot ourselves?**
14. **Can TinyCC perform the same cross-compilation workflow with downloadable host-native target-capable libraries?**
15. **What are the minimum files required for a Windows → Linux x64 build?**
16. **What are the minimum files required for Windows → Linux ARM64?**
17. **What should remain permanently bundled with Chemical?**
18. **What should be downloaded per architecture?**
19. **What should be downloaded per OS/ABI?**
20. **What should be cached per target?**
21. **What is the estimated size of each resulting package?**
22. **What should we implement first to prove the architecture?**

---

# 24. Most Important Instruction

**Analyze the actual Chemical codebase before making architectural claims.**

Do not give a generic LLVM cross-compilation tutorial.

We specifically need to know:

> Given how Chemical is currently implemented, what exact changes are necessary to achieve our desired cross-compilation architecture while minimizing the size of the base LLVM/compiler installation?

Trace the existing code, build scripts, LLVM initialization, target registration, Clang invocation, LLD invocation, TinyCC integration, generated C, runtime/linking, and packaging.

Where possible, provide exact:

* files;
* classes;
* functions;
* CMake targets;
* build options;
* libraries;
* dependencies;
* initialization calls;
* dynamic-loading boundaries.

Clearly distinguish confirmed facts from assumptions and things that require an experiment.

The ultimate desired architecture is:

```text
                         Chemical
                            |
                    target = linux-x64
                            |
             +--------------+--------------+
             |                             |
             v                             v
          TinyCC                         LLVM
             |                             |
       host-native backend          common LLVM core
             |                             |
             |                       target backend
             |                             |
             v                             v
       target object                 target object
             |                             |
             +--------------+--------------+
                            |
                      target linker
                            |
                     target runtime
                            |
                            v
                   Linux x64 executable
```

with target-specific components downloadable and cacheable rather than forcing every Chemical installation to contain every possible target.

The analysis should optimize for **correctness first, then minimal distribution size, while preserving Chemical's self-contained compiler philosophy**.
