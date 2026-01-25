# Chemical Language Syntax Reference

This document provides a comprehensive reference for Chemical language syntax, designed to help AI systems generate correct Chemical code. All syntax described here is based on analysis of the parser implementation and existing codebase.

## Table of Contents

1. [Core C-like Features](#core-c-like-features)
2. [Standard Library](#standard-library)
3. [Advanced Features](#advanced-features)
4. [Best Practices for AI Generation](#best-practices-for-ai-generation)

## Basic Structure

### File Extensions
- Source files: `.ch`
- Module descriptor: `chemical.mod`

### Module Declaration
```chemical
module module_name

source "src"  // Include all .ch files in src directory
import module_name  // Import other modules
```

**Important**: No import statements are required at the top of `.ch` files. Any `.ch` file can access anything in the current module and from any modules imported in `chemical.mod`. Import statements are only valid in `build.lab` or `chemical.mod` files.

```chemical
// Mutable variables
var name : type = value
var name = value  // Type inference

// Immutable constants
const name : type = value
const name = value  // Type inference

// Compile-time variables
comptime var name : type = value
comptime const name : type = value
```

#### Variable Initialization Rules
- Local variables must be initialized at declaration
- Global/top-level variables can be declared without initialization if type is specified
- `const` variables must be initialized at declaration
- Type inference available when value is provided

#### Examples
```chemical
var x : int = 42
var message = "Hello, World!"
const PI = 3.14159
comptime var COMPTIME_VAL = 100
```

### Basic Types

#### Primitive Types
```chemical
// Chemical integer types
i8, i16, i32, i64, int128
u8, u16, u32, u64, uint128

// C-compatible types
char, short, int, long, longlong
uchar, ushort, uint, ulong, ulonglong

// Floating point
float, double, longdouble, float128

// Other types
bool, void, any
```

#### Pointers and References
```chemical
var ptr : *int          // Pointer to int
var ptr_ptr : **int     // Pointer to pointer to int
var func_ptr : () => int // Function pointer
var ref : &int          // Reference to int
```

#### Arrays
```chemical
var arr : [10]int       // Fixed-size array
var slice : []int       // Slice (dynamic array)
var multi : [10][20]int // Multi-dimensional array
```

### Control Flow

#### If Statements
```chemical
if(condition) {
    // then block
} else if(other_condition) {
    // else if block
} else {
    // else block
}

// If as expression
var result = if(condition) { value1 } else { value2 }
```

#### Switch Statements
```chemical
switch(value) {
    case1 => {
        // case 1 block
        // break statement NOT allowed
    }
    case2 => {
        // case 2 block
    }
    default => {
        // default block
    }
}

// Switch as expression
var result = switch(value) {
    case1 => value1
    case2 => value2
    default => default_value
}
```

#### Loops
```chemical
// While loop
while(condition) {
    // loop body
}

// Do-while loop
do {
    // loop body
} while(condition)

// For loop
for(var i = 0; i < 10; i++) {
    // loop body
}

// Loop as expression
var result = loop {
    // loop body
    break value
}
```

#### Break and Continue
```chemical
for(var i = 0; i < 10; i++) {
    if(condition) {
        break
    }
    if(other_condition) {
        continue
    }
}
```

### Functions

#### Basic Function Definition
```chemical
func function_name(param1 : type1, param2 : type2) : return_type {
    // function body
    return value
}

// Public functions (exposed to other modules)
public func public_function(param : type) : return_type {
    // implementation
}
```

#### Function Parameters
```chemical
// Regular parameters
func func(param : type) : return_type

// Default values
func func(param : type = default_value) : return_type

// Variadic parameters, only for c extern defined functions
func func(param : type, ...) : return_type

// Implicit parameters
func first(param : type) : return_type
```

### Structs

#### Basic Struct Definition
```chemical
// publib before would expose to other modules
struct StructName {
    // Members with access specifiers
    public var member1 : int
    private const member2 : std::string
    
    // Default values
    var member3 : float = 0.0
    
    // Methods
    func method_name(&self) : return_type {
        // method body
    }
}
```

#### Struct Members
- Fields: `var name : type` or `const name : type`
- Methods: `func name(params) : return_type`
- Constructors: Use `@constructor` annotation

#### Destructors
```chemical
struct ManagedResource {
    var resource : *void
    
    @delete
    func destruct(&self) {
        unsafe {
            // destruct statement just calls free under the hood
            destruct self.resource
        }
    }
}
```

### Unsafe Code

#### Unsafe Blocks
```chemical
func unsafe_operation() {
    unsafe {
        // Unsafe operations here
        var ptr = malloc(100) as *int
        *ptr = 42
        // destruct statement just calls free under the hood
        destruct ptr;
    }
}
```

#### Memory Management
```chemical
func manual_memory() {
    unsafe {
        var ptr = malloc(100) as *int
        *ptr = 42
        
        // Use pointer
        // dealloc statement just calls free under the hood
        dealloc ptr;

        // for structs use destruct instead of dealloc
        // destruct ensures that destructors are called
    }
}
```

#### New and Dealloc
```chemical
func dynamic_allocation() {
    unsafe {
        var ptr = new int  // Allocate and construct
        *ptr = 42
        dealloc ptr      // Deallocate

        // for structs use destruct instead of dealloc
        // destruct ensures that destructors are called
    }
}
```

### Variants and Pattern Matching

#### Basic Variant Definition
```chemical
variant VariantName {
    Case1(param1 : type1, param2 : type2)
    Case2(param : type)
    Case3()  // Empty case
}
```

#### Pattern Matching

**Strict Rule: Pattern Matching only works with variants**

##### Variable Pattern Matching
```chemical
variant Option<T> {
    Some(value : T)
    None()
}

func get_value(opt : Option<int>) : int {
    var Some(value) = opt else -1
    return value
}
```

##### Pattern Matching with Return
```chemical
func get_value_or_return(opt : Option<int>) : int {
    var Some(value) = opt else return 0
    return value
}
```

##### Pattern Matching with Unreachable
```chemical
func unwrap(opt : Option<int>) : int {
    var Some(value) = opt else unreachable
    return value
}
```

##### Pattern Matching in If Statements
```chemical
func check_option(opt : Option<int>) : bool {
    if(var Some(value) = opt) {
        return value > 0
    }
    return false
}
```

##### Switch Pattern Matching
```chemical
func process_variant(variant : MyVariant) : int {
    switch(variant) {
        Case1(value) => {
            return value * 2
        }
        Case2 => return -1
    }
}
```

##### Type Testing with `is`
```chemical
func is_some_case(opt : Option<int>) : bool {
    return opt is Option.Some
}

func is_none_case(opt : Option<int>) : bool {
    return opt is Option.None
}
```

**Note**: These core C-like features are sufficient to write good, performant Chemical code. Stick to these features for better reliability and performance.

## Standard Library

The Chemical standard library provides essential data structures and utilities. Below are the key APIs that AI systems should know.

### std::string

**Important**: You cannot use the `+` operator between `std::string` or `*char`. Chemical doesn't perform magic behind the scenes like some other languages.

#### Constructor
```chemical
var s = std::string()  // Empty string
```

#### Methods
```chemical
// Append a single character
s.append('x')

// Append a string view
s.append_view(std::string_view("wow"))
s.append_view("hello world")  // string_view has implicit constructor for *char

// Append another std::string
s.append_view(other_string)
```

#### Usage Examples
```chemical
var s = std::string()
s.append('H')
s.append_view("ello")
s.append(' ')
s.append_view("World")

// WRONG - This will not work:
// var result = s + " more text"  // + operator not overloaded

// CORRECT:
s.append_view(" more text")
```

### std::string_view

A lightweight view into string data without ownership.

#### Constructor
```chemical
var view = std::string_view("hello")  // From *char literal
var view2 = std::string_view(string_obj)  // From std::string
```

#### Usage
```chemical
func process_text(text : std::string_view) {
    // Process text without copying
}

process_text("hello")  // Works due to implicit constructor
```

### std::vector

Dynamic array container similar to C++ std::vector.

#### Constructor
```chemical
var vec = std::vector<int>()  // Empty vector of int
var vec2 = std::vector<std::string>()  // Empty vector of strings
```

#### Methods
```chemical
// Add elements
vec.push_back(42)
vec.push_back(100)

// Access elements
var first = vec[0]  // No bounds checking
var safe_first = vec.at(0)  // With bounds checking

// Size
var size = vec.size()

// Check if empty
var empty = vec.empty()

// Clear all elements
vec.clear()

// Reserve capacity
vec.reserve(100)  // Allocate space for 100 elements
```

#### Usage Examples
```chemical
var numbers = std::vector<int>()
for(var i = 0; i < 10; i++) {
    numbers.push_back(i * i)
}

var strings = std::vector<std::string>()
strings.push_back(std::string())
strings[0].append_view("first")
```

### std::unordered_map

Hash map container for key-value pairs.

#### Constructor
```chemical
var map = std::unordered_map<std::string, int>()
var map2 = std::unordered_map<int, std::string>()
```

#### Methods
```chemical
// Insert key-value pairs
map.insert("key", 42)
map["another_key"] = 100  // Operator[] for insertion/access

// Access elements
var value = map["key"]  // Returns default value if not found
var opt_value = map.find("key")  // Returns optional

// Check if key exists
var exists = map.contains("key")

// Remove elements
map.erase("key")

// Size
var size = map.size()

// Clear all elements
map.clear()
```

#### Usage Examples
```chemical
var word_count = std::unordered_map<std::string, int>()

// Count words
word_count["hello"] = 1
word_count["world"] = 2

// Check and increment
if(word_count.contains("hello")) {
    word_count["hello"] = word_count["hello"] + 1
}

// Iterate (if iteration is supported)
// for(var [key, value] in word_count) {
//     printf("%s: %d\n", key.data(), value)
// }
```

### std:: Option and Result Types

The standard library provides commonly used variant types for error handling and optional values.

#### std::Option
```chemical
// Already available in standard library
var opt = std::Option<int>.Some(42)
var empty = std::Option<int>.None()

// Use with pattern matching
var Some(value) = opt else -1
```

#### std::Result
```chemical
// Already available in standard library  
var success = std::Result<int, std::string>.Ok(42)
var error = std::Result<int, std::string>.Err("failed")

// Use with pattern matching
var Ok(value) = success else -1
```

### Performance Guidelines

1. **Prefer *char for simple strings** - Use `*char` for C-style string operations when possible
2. **Use string_view for read-only access** - Avoid copying when you just need to read string data
3. **Reserve vector capacity** - Pre-allocate memory when you know the expected size
4. **Avoid excessive string concatenation** - Use `append_view()` instead of `+` operator
5. **Use appropriate containers** - Choose `vector` for sequential access, `unordered_map` for key-value lookup

## Advanced Features

These features provide additional functionality but should be used judiciously. For most use cases, the core C-like features and standard library are sufficient.

### Generics

#### Generic Type Parameters
```chemical
struct GenericStruct<T> {
    var value : T
}

func <T> generic_func(param : T) : T {
    return param
}
```

#### Generic Constraints
```chemical
func <T> constrained_func(param : T) : T 
    where T : SomeInterface 
{
    return param
}
```

#### Default Type Parameters
```chemical
struct DefaultGeneric<T = int> {
    var value : T
}

func <T = std::string> default_generic_func(param : T) : T {
    return param
}
```

#### Generic Instantiation
```chemical
var int_struct = GenericStruct<int>
var string_struct = GenericStruct<std::string>
var default_struct = DefaultGeneric  // Uses int as default
```

### Lambda Functions

#### Basic Lambda
```chemical
var lambda = (param : type) => return_type {
    return param * 2
}
```

#### Capturing Lambda
```chemical
var x = 10
var capturing_lambda = |x|(param : int) => int {
    return param + x
}
```

#### Lambda without Parameters
```chemical
var no_params = () => int {
    return 42
}
```

#### Function Types
```chemical
// Function pointer type
var func_ptr : (int, std::string) => bool

// Lambda type with capture - only capturing functions can capture variables
var lambda_type : std::function<(param : type) => return_type>
```

### Annotations

#### Built-in Annotations
```chemical
@deprecated          // Marks item as deprecated
@extern             // External definition
@no_mangle          // Don't mangle name
@static             // Static member
@implicit           // Implicit parameter
@constructor        // Constructor method
@test               // Test function
@inline             // Inline function
@noinline           // Don't inline
```

#### Annotation Usage
```chemical
@deprecated("Use new_function instead")
func old_function() {
    // implementation
}

@extern public func printf(format : *char, ...) : int

@constructor
func init(&self, param : int) {
    // constructor implementation
}

@test
func test_feature() : bool {
    return true
}
```

### Conditional Compilation
```chemical
comptime if(def.windows) {
    // windows specific code
    // the syntax is still checked
    // however type checking is skipped if def.windows is false
}
```

### Compile-time Features

#### Compile-time Functions
```chemical
// only if function can run at compile time
comptime func calculate_at_compile_time() : int {
    return 42 * 2
}

const RESULT = calculate_at_compile_time()
```

#### Compile-time Conditionals
```chemical
comptime if(condition) {
    // Compile-time branch
} else {
    // Other compile-time branch
}
```

### Interoperability

#### C Function Declarations
```chemical
@extern public func printf(format : *char, ...) : int
@extern public func malloc(size : usize) : *void
@extern public func free(ptr : *void) : void
```

#### C Struct Definitions
```chemical
@extern
struct CStruct {
    var field1 : int
    var field2 : *char
}
```

#### Type Casting
```chemical
var int_val = 42
var float_val = int_val as float
var ptr_val2 = &mut int_val as *mut float
```

### Move Semantics
```chemical
func move_example() {
    var original = MyStruct()
    // implicit moves
    var moved = original  // Move ownership
    
    // original is no longer valid here
}
```

### Generic Structs and Variants

#### Generic Structs
```chemical
struct GenericStruct<T, U> {
    var value1 : T
    var value2 : U
    
    func <V> generic_method(param : V) : V {
        return param
    }
}
```

#### Generic Variants
```chemical
variant GenericVariant<T> {
    Some(value : T)
    None()
}

variant MultiGeneric<T, U> {
    First(a : T, b : U)
    Second(value : T)
    Empty()
}
```

#### Variant with Default Type Parameter
```chemical
variant VariantWithDefault<T = int> {
    Some(value : T)
    None()
}
```

#### Variant Methods
```chemical
variant MyVariant {
    Case1(value : int)
    Case2(text : std::string)
    
    func get_value(&self) : int {
        switch(self) {
            Case1(value) => return value
            Case2 => return -1
        }
    }
}
```

## Best Practices for AI Generation

### Core Principles
1. **Write C-like code for performance** - Chemical is very C-like, strings are `*char`. For performant code, stick to C-like features with Chemical syntax.
2. **Use structs and destructors for better API** - Combine C-style performance with modern safety through destructors.
3. **Add variants and pattern matching selectively** - Use these for specific modern features, but don't overuse them.
4. **No magic behind the scenes** - Chemical doesn't perform hidden allocations or optimizations like some higher-level languages.

### Bad AI Patterns to Avoid
1. **Don't use + operator with strings** - `std::string` and `*char` don't support `+` operator. Use `append()` functions specifically designed.
2. **No import statements in .ch files** - Import statements are only valid in `build.lab` or `chemical.mod` files.
3. **No defer statements** - Chemical doesn't support defer (may be added in future).
4. **Don't expect string concatenation magic** - Use `append_view()` for efficient string building.

### Code Quality Guidelines
1. **Always specify types for struct members** - Don't rely on inference in struct definitions
2. **Use proper access specifiers** - Be explicit about public/private/internal
3. **Initialize variables** - Local variables must be initialized at declaration
4. **Handle all variant cases** - When using switch with variants, cover all cases
5. **Use pattern matching** - Prefer pattern matching over manual variant checking
6. **Mark unsafe code** - Always wrap unsafe operations in unsafe blocks
7. **Include proper annotations** - Use @extern for C functions
8. **Follow naming conventions** - Use PascalCase for types, camelCase for functions
9. **Handle memory properly** - Use destructors for resource cleanup
10. **Use generics appropriately** - Don't over-genericize without need

### Performance Guidelines
1. **Prefer core C-like features** - For best performance, stick to the basics
2. **Use pointers when appropriate** - Chemical supports pointers for low-level control
3. **Leverage destructors for RAII** - Get modern safety with C-like performance
4. **Use variants judiciously** - Great for certain use cases but not a replacement for all enums
5. **Standard library when beneficial** - Use std containers when they provide clear advantages

## Common Patterns

### Option Type Pattern

std::Option type is available in standard library

```chemical
variant Option<T> {
    Some(value : T)
    None()
}

func <T> unwrap_or(opt : Option<T>, default : T) : T {
    var Some(value) = opt else default
    return value
}
```

### Result Type Pattern

std::Result type is available in standard library

```chemical
variant Result<T, E> {
    Ok(value : T)
    Err(error : E)
}

func <T, E> map_result(result : Result<T, E>, mapper : (T) => T) : Result<T, E> {
    switch(result) {
        Ok(value) => Result.Ok(mapper(value))
        Err(error) => Result.Err(error)
    }
}
```

### Builder Pattern
```chemical
struct Builder {
    var field1 : int = 0
    var field2 : std::string = ""
    
    func with_field1(value : int) -> Builder {
        field1 = value
        return this
    }
    
    func with_field2(value : std::string) -> Builder {
        field2 = value
        return this
    }
    
    func build() -> MyStruct {
        return MyStruct(field1, field2)
    }
}
```

This syntax reference provides the foundation for generating correct Chemical code. The language emphasizes safety, performance, and C interoperability while maintaining modern language features.
