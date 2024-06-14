### What is CBI

CBI is Compiler Binding Interface, It allows source code written by the user to interact with the compiler.
For example, User can write code, that'll be used by the compiler to process a macro.

### Usages of CBI?

When you write in our language the following code

```chemical
var x = #html {
    <p>
        <h1>Heading</h1>
        </span>My Subtitle</span>
    </p>
}
```

Here a `cbi` named `html` is searched by the compiler at compile time, to parse the html code.
CBI's have many more usages apart from this, They allow us to invoke code inside compiler and are very capable of running at runtime.
So if you write a great CBI, you can run it at runtime to parse user's html code yourself.

### Learning CBI

You must remember the following basic annotations in CBI

- `@cbi:global("name_of_cbi")` 
  - takes upcoming function / struct to be collected by another CBI
- `@cbi:create("html")`
  - creates a cbi named `html`, will cause error if already exists
- `@cbi:import("html", "compiler")`
  - imports collected things (functions / structs) (collected by cbi:global or some other annotation) into cbi named `html`
- `@cbi:to("html")`
  - put's upcoming thing (function / struct) into a cbi named `html`
- `@cbi:compile("html")`
  - compiles html cbi so it can be used by the user, nodes can not be added to the cbi after this.
- `@cbi:begin("html")`
  - if you are tried of using `@cbi:to` annotations to put structs / functions into CBI, you can put `@cbi:begin` right after `@cbi:create`, and every struct / function will be put into cbi until one of
  `@cbi:compile` or `@cbi:end` is found
- `@cbi:end()`
  - end collection of structs / functions into a CBI, every end annotation must have a corresponding `@cbi:begin` annotation
  that started it

#### Example

```chemical
import "@compiler/Lexer.h" // this import allows us to use @cbi:import("number", "compiler")

// the ordering is importing, create should be called first and then import
@cbi:create("number");
@cbi:import("number", "compiler");

@dispose(false)
func printf(format : string, args : any...);

@cbi:to("number")
struct number {
    func lex(lexer : Lexer*) {
        var ln = lexer.provider.getLineNumber(); // zero based
        var cn = lexer.provider.getLineCharNumber(); // zero based
        // you can parse it using your code
        // chemical provides it's own API, which allows parsing chemical code
        var num = lexer.provider.readNumber();
        printf("look ma I read a number : %s at line %d,%d", num, ln + 1, cn + 1);
    }
}

@cbi:compile("number");

func main() {
    #number { 123 }
    printf("hello world");
}
```

Here a number cbi is being created to parse a number, well once a number is parsed, if it's not given a type, chemical considers it by default `int` if it can fit within it.

Since chemical finds that the token you lexed is a number token that belongs to chemical programming language, It converts it to appropriate type / value.
If you want to lex your own tokens, it's really easy, you just have to call `lexer.put(&token)`

I'm mixing lexing and parsing, that's because chemical does lexing and parsing of code, in one go.
Chemical creates a CST instead of AST. Chemical allows you lex any code you desire, in any way you want, The entire C language is at your command here. Chemical doesn't enforce it's API on you
unless you need to use it.

The import `cbi:import("number", "compiler")` will put global cbi node called compiler into number, The compiler node is given by chemical, which allows you
to have access to chemical's compiler API, It defines useful structs and function types, so that you can call them from your code.

The `cbi:globa("name_of_cbi")` is just like `@cbi:to("number")`, It's put above functions / structs, to collect them, so that they can be imported by other CBI's.

Now to try it out, you should use `@cbi:begin("html")` instead of `@cbi:to("html")`, once begin has been detected, every struct / function you write will be part of the CBI.
Play around and create a global variable that goes in CBI, on every invocation increment and print it out

### How does CBI works ?

When you use `@cbi:to("name_of_cbi")` you are telling compiler to collect the function / struct into cbi html, and compiler parses the function / struct, then when you've collected all the functions / structs you require for your CBI, now `cbi:compile("name_of_cbi")` is called
to compile everything you've collected at compile time (pre compiling), just in time compilation, so that functions inside it could be invoked.


When compiler detects the `#number { 123 }` It checks if number is a CBI created by the user, now when it's found, It's lex function is invoked
present inside the struct of it's own name, so `number.lex`.

### Runtime CBI and Node Disposing

When you use cbi annotations, the nodes that are put into cbi, aren't considered for compilation, meaning the nodes are retained upto compile time
and nodes (functions / structs) are disposed after use at compile time, they don't end up in the runtime code

> A Node is anything that is part of the code (AST), like a function or a struct or global variable declaration or an enum

Sometimes however, you may want these nodes to end up in the runtime code, Because you want to do some runtime processing
using the same functions inside CBI.

Here are key annotations to help you accomplish this

- `@dispose(true)`
  - if true, disposes a node at compile time, no trace of it at runtime
  - if false, avoids disposing a node at compile time, so it's available at runtime 
- `@dispose:begin()`
  - every node after this annotation will be disposed
- `@dispose:end()`
  - every node after this annotation will not be disposed

When you use a cbi annotation like `@cbi:begin`, It automatically calls `@dispose:begin` under the hood, making every node disposable.
This helps us reduce code that is part of compile time by default.

Similarly `@cbi:to` and `@cbi:global` use `@dispose:false` under the hood. 

To override this behavior you must use dispose annotations as they suit your need to avoid disposing your nodes when using cbi annotations 

### CBI module

You can make an entire module CBI in the `.lab` file, you don't need to put annotations like `@cbi:to` in that case.
This module will be pre-compiled in a single go.

This improves performance, because every function you write is expected to be collected, so compiler does not have to deal with
code that can be mixed with code that isn't for CBI.

This module won't support any runtime code or any usage of the cbi that it aims to create.

If user wants it to be part of runtime, user can do that in the `.lab` file as well

This approach improves compilation time, The con of this approach is that you lose specificity in disposing nodes, or making them part of CBI which is a very minor con.