# C Integer Promotion Safety

## The Problem

When generating C code for narrow signed integer types (i8, i16), the Chemical compiler must account for **C's integer promotion rules**. In C, any integer type narrower than `int` (i8, i16, u8, u16, etc.) is promoted to `int` before any arithmetic or bitwise operation. This means the truncation behavior of narrow types (wrapping on overflow) is **not automatically preserved** in the generated C code.

## Concrete Example

Consider this Chemical code:

```chemical
var x : i16 = -32768i16
var diff = x - 1i16
```

The expected result, following two's complement wrapping for `int16_t`, is:

```
-32768 - 1 = -32769  →  wrapped to int16_t  →  32767
```

However, the C codegen generates:

```c
int16_t x = (int16_t)(32768);    // = -32768
int16_t diff = (x - 1);           // C promotes to int: (-32768) - 1 = -32769
```

And the comparison:

```c
diff == 32767;  // (int)(32767) == 32767 → TRUE (works because diff is int16_t)
```

This works **only** when the intermediate result is stored in an `int16_t` variable, which causes the truncation. If the expression is inlined (e.g., `return (x - 1) == 32767;`), the truncation is lost:

```c
return ((x - 1) == 32767);  // (int)(-32769) == 32767 → FALSE! Bug!
```

## Root Cause

The Chemical compiler's C codegen (`preprocess/2c/2cASTVisitor.cpp`) does **not** always add explicit type truncation casts `(intN_t)` around expressions of narrow integer types. The truncation only happens implicitly when the expression result is:
1. Assigned to a variable of the narrow type (`int16_t diff = ...`)
2. Explicitly cast in the source code (`diff as i16`)

When the expression is used directly in a comparison or as a return value from a lambda, the C codegen may elide intermediate variables and inline the expression, losing the truncation.

## Test Design Principle

To write tests that work correctly with C codegen:

1. **Use intermediate typed variables** for arithmetic on narrow types:
   ```chemical
   var x : i16 = -32768i16
   var y : i16 = 1i16
   var sum : i16 = x + y     ← explicit i16 type ensures truncation
   return sum == -32767i16
   ```

2. **Avoid inlining narrow-type arithmetic** in comparisons:
   ```chemical
   // BAD - may inline and lose truncation:
   return (x - 1i16) == 32767i16
   
   // GOOD - forces truncation via intermediate variable:
   var diff : i16 = x - 1i16
   return diff == 32767i16
   ```

3. **Prefer non-overflow arithmetic** where possible:
   ```chemical
   // SAFE: (-32768) + 1 = -32767 (no overflow)
   var sum = x + 1i16
   return sum == -32767i16
   
   // UNSAFE: x - 1 overflows i16, relies on truncation
   var diff = x - 1i16
   return diff == 32767i16
   ```

## Affected Patterns

| Pattern | Risk | Explanation |
|---------|------|-------------|
| `var v = a + b; return v == c;` | ✅ Safe | Variable assignment forces truncation |
| `return (a + b) == c;` | ⚠️ Unsafe | Expression may be inlined without truncation |
| `a + b` in lambda body | ⚠️ Unsafe | Lambda body may elide intermediate variables |
| `var v : T = a + b` (explicit type) | ✅ Safe | Explicit type annotation forces truncation |
| `var v = a + b` (inferred type) | ⚠️ Risky | Inferred type depends on expression type |

## Future Work

The proper fix would be to add explicit type truncation casts in the C codegen for all expressions whose Chemical type is narrower than `int`. This would ensure safe codegen regardless of how the expression is used (variable assignment, inline, lambda body, etc.).

The fix location is in expression codegen functions within `preprocess/2c/2cASTVisitor.cpp`. For any expression whose type is i8, i16, u8, or u16, the codegen should wrap the expression with the appropriate cast, e.g.:

```c
// Current:
x - 1

// Fixed:
(int16_t)(x - 1)
```
