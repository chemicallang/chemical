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

### Specifying Source Paths

You must add chemical sources the module requires to be compiled.

```chmod
application my_mod

source "src"
```

The `source` command takes the path, and includes that path, If the path
1 - points to a directory, we find all the chemical source files (.ch extension) recursively and add it to compilation
2 - points to a file, we add that file for compilation (.ch extension)

#### Conditional Paths

Suppose you want to include a directory only on windows, because it uses windows APIs

```chmod
source "src"
source "win" if windows
source "posix" if posix
```

Here `win` directory will only be added on windows, `posix` on posix systems, and `src` on both. This allows us to be cross platform, we don't have preprocessor, although we do have conditional comptime if statements that allow you to write platform specific code.

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

#### Subdirectories in a MonoRepo

If you want to import a module from a subdirectory of a repository, you can do it like this

```chmod
import "github.com/owner/repo" subdir "subdirectory"
```

This will download the repository and compile it as a module, It will look for a `chemical.mod` file or a `build.lab` file in the subdirectory of the repository.

Note that the entire repo would be downloaded for that single directory, This is only
useful if many tiny libraries interop with each other, and if user requires one, he is very likely to need others.

For example, We ship sokol bindings this way, Sokol repo contains all the libraries in a single repo, dividing them into multiple repos would cause difficulty in maintainance.

#### Version Pinning

You can pin the version of the module like this

```chmod
import "github.com/owner/repo" version "1.0.0"
```

This will download the repository and compile it as a module, It will look for a `chemical.mod` file or a `build.lab` file in the root of the repository.

If there are two versions of the same library, A conflict occurs, compiler will try to resolve the conflict by parsing the versions, which it expects to be semantic versions.

If one version is greater than the other, The newer version is kept and older is discarded

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