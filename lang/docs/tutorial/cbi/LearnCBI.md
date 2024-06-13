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

You must remember the following 5 basic annotations in CBI

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

#### Example

```chemical
import "@compiler/Lexer.h" // this import allows us to use @cbi:import("number", "compiler")

// the ordering is importing, create should be called first and then import
@cbi:create("number");
@cbi:import("number", "compiler");

@cbi:to("number")
struct number {
    func lex(lexer : Lexer*) {
        // you can parse it using your own api
        // chemical provides it's own api, which allows parsing chemical code
        var num = lexer.provider.readNumber();
        printf("look ma I read a number : %d", num);
    }
}

@cbi:compile("number");

func main() {
    var num = #number { 123 }
    printf("checkout the number : %d", num);
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

### How does CBI works ?

When you use `@cbi:to("name_of_cbi")` you are telling compiler to collect the function / struct into cbi html, and compiler parses the function / struct, then when you've collected all the functions / structs you require for your CBI, now `cbi:compile("name_of_cbi")` is called
to compile everything you've collected at compile time (pre compiling), just in time compilation, so that functions inside it could be invoked.


When compiler detects the `#number { 123 }` It checks if number is a CBI created by the user, now when it's found, It's lex function is invoked
present inside the struct of it's own name, so `number.lex`.

### CBI module

You can make an entire module CBI in the `.lab` file, you don't need to put annotations like `@cbi:to` in that case.
This module will be pre-compiled in a single go.

This improves performance, because every function you write is expected to be collected, so compiler does not have to deal with
code that can be mixed with code that isn't for CBI.

This module won't support any runtime code or any usage of the cbi that it aims to create.