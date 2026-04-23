# Improving Developer Experience in Universal CBI

## Overview

The `#universal` component system is powerful for building reactive web applications with server-side rendering support. However, several developer experience (DX) pain points have emerged that make it harder than necessary for developers to build scalable apps. This document outlines the problems we've encountered and proposes solutions to improve the API and reduce friction.

## Current Pain Points

### 1. Manual `.value` Unwrapping for Reactive Variables

**Problem:**
When passing reactive variables (state/computed) as props or using them in expressions, developers must manually append `.value` to access the actual value. This is unintuitive and error-prone.

```chemical
#universal MyComponent(props) {
    state items = [1, 2, 3]
    state selected = 0
    
    // ❌ This fails - items is a reactive wrapper, not an array
    {items.map(item => <div>{item}</div>)}
    
    // ✅ Must write .value explicitly
    {items.value.map(item => <div>{item}</div>)}
    
    // ❌ Also fails when accessing nested properties
    var count = items.length  // undefined!
    
    // ✅ Must unwrap first
    var count = items.value.length
}
```

**Impact:**
- Developers don't expect to write `.value` in JSX expressions
- Common errors: "Cannot read properties of undefined (reading 'map')"
- Requires developers to understand the internals of reactivity
- Error messages are confusing because the runtime error doesn't point to the root cause

**Solutions:**
1. **Automatic unwrapping in JSX expressions** (Compiler responsibility)
   - Detect reactive variables in JSX and automatically insert `.value`
   - Track all state/computed variables to identify them in expressions
   
2. **Clearer documentation and error messages**
   - Point developers to reactive variable requirements upfront
   - Improve compiler errors to suggest `.value` when accessing reactive variables
   
3. **TypeScript integration** (if using TS)
   - Type reactive wrappers to make `.value` requirement obvious at development time

---

### 2. Inconsistent Props Passing Semantics

**Problem:**
When passing reactive objects as props, developers can accidentally pass unwrapped values (`.value`) instead of the reactive object itself, breaking reactivity in child components.

```chemical
// app.ch - Parent component
#universal App(props) {
    state store = { categories: [], notes: [] }
    
    // ❌ Passing unwrapped value - child won't see updates
    <TimelineView store={store.value} />
    
    // ✅ Pass reactive object
    <TimelineView store={store} />
}

// timeline.ch - Child component
#universal TimelineView(props) {
    // Now must unwrap here
    var categories = props.store.value.categories
}
```

**Impact:**
- Inconsistent behavior between components
- Difficult to debug: changes in parent don't propagate to child
- No clear convention for when to unwrap
- Requires developers to understand prop passing semantics deeply

**Solutions:**
1. **Establish clear prop passing convention**
   - Document: "Always pass reactive objects, never `.value`"
   - Use TypeScript/validation to enforce this at component boundaries
   
2. **Compiler-enforced unwrapping in child components**
   - When accessing props that are reactive, automatically insert `.value`
   - Track which props are reactive based on parent component's state
   
3. **Runtime validation**
   - Warn developers if a reactive object is being passed when a plain value is expected
   - Detect when `.value` is passed when a reactive object was intended

---

### 3. Reactive Objects Require `.value` at Multiple Levels

**Problem:**
When reactive objects contain nested properties, developers must chain `.value` accesses, leading to verbose and hard-to-read code.

```chemical
#universal UserProfile(props) {
    // ❌ Confusing - multiple levels of .value
    var userName = props.user.value.user.value.name
    
    // ✅ Cleaner, but still requires manual unwrapping
    var userName = props.user.value.user.name
    
    // Even accessing in JSX requires .value
    <div>{props.user.value.user.name}</div>
}
```

**Impact:**
- Code readability suffers
- Easy to miss one `.value` and get runtime errors
- Props interface becomes opaque
- Developers can't tell from the code structure what's reactive vs. plain

**Solutions:**
1. **Destructuring with automatic unwrapping**
   - Allow `var { user: { name } } = props` to automatically unwrap
   - Syntax sugar that handles nested reactive objects
   
2. **Computed properties for derived state**
   - Use computed fields to expose flattened, unwrapped data
   - `computed(() => props.user.value.user.name)`
   
3. **Clearer prop typing**
   - Type system should document that `props.user` is reactive
   - IDEs can help developers navigate the structure

---

### 5. Array Methods on Reactive Arrays

**Problem:**
Array methods (`.map()`, `.filter()`, `.reduce()`) don't work directly on reactive arrays. Each requires unwrapping.

```chemical
#universal NoteList(props) {
    state notes = []
    
    // ❌ Fails - notes is reactive wrapper, not array
    {notes.map(note => <NoteCard note={note} />)}
    
    // ✅ Must unwrap
    {notes.value.map(note => <NoteCard note={note} />)}
    
    // Same issue with other array methods
    var count = notes.filter(n => n.pinned).length  // ❌ Fails
    var count = notes.value.filter(n => n.pinned).length  // ✅ Works
}
```

**Impact:**
- Very common pattern in React/Preact development
- Developers expect this to "just work"
- Requires understanding reactivity internals
- Error messages are generic and unhelpful

**Solutions:**
1. **Proxy array methods at compile time**
   - When encountering `reactive_var.map(...)`, transform to `reactive_var.value.map(...)`
   - Apply to common methods: map, filter, reduce, find, some, every, forEach
   
2. **Reactive array wrapper with methods**
   - Make reactive arrays actually delegate array methods
   - `state items = reactive([1,2,3])` returns object with `.map()` etc that work directly
   
3. **Better error messages**
   - When `.map` is called on non-array, suggest checking if it's a reactive variable
   - Point to `.value` property or unwrapping

---

### 6. Unclear Component Boundaries and Data Flow

**Problem:**
It's not always clear which props are reactive and which are plain values. This leads to confusion about where to use `.value`.

```chemical
// Parent
#universal App(props) {
    state items = []
    var count = 5  // Plain value
    
    // Which are reactive? Not obvious from usage
    <ItemList items={items} count={count} onAdd={handleAdd} />
}

// Child - unclear what's reactive
#universal ItemList(props) {
    // Is props.items reactive? Is props.onAdd?
    // Developer has to check parent or add .value everywhere defensively
    {props.items.value.map(...)}  // Maybe should be .value?
    props.onAdd()  // Or is this wrapped?
}
```

**Impact:**
- Inconsistent prop usage across codebase
- Defensive programming (adding `.value` everywhere)
- Makes refactoring difficult
- Type system can't help without explicit annotations

**Solutions:**
1. **Explicit prop type annotations**
   - `#universal MyComponent(props: { items: Reactive<Array>, count: number })`
   - TypeScript-like syntax to specify which props are reactive
   
2. **Compiler warnings for mixed reactivity**
   - Warn when reactive and non-reactive props are used inconsistently
   - Suggest using consistent patterns
   
3. **Documentation and linting**
   - Create linter rule: "Always annotate reactive props"
   - Provide examples of proper prop interfaces

---

## Recommended Improvements

### Phase 1: Compiler Enhancements (High Impact, Medium Effort)

1. **Auto-unwrapping in expressions**
   - Detect state/computed variables used in array methods
   - Transform `.map()` → `.value.map()` automatically
   - Apply to all common array operations

2. **Better error messages**
   - When `.map is not a function` error occurs on state variable, suggest `.value`
   - Add compile-time warnings for reactive variable usage


4. **Reactive variable detection**
   - Track all state/computed variables through the component
   - Provide IDE hints about which variables are reactive

### Phase 3: API Design (High Impact, High Effort)

5. **Clearer prop semantics**
   - Introduce explicit `Reactive<T>` type for reactive props
   - Document prop passing conventions in template syntax
   - Add validation at component boundaries

6. **Destructuring support**
   - Allow `var { user: { name } } = props` with automatic unwrapping
   - Syntax sugar for common patterns

### Phase 4: Documentation (Medium Impact, Low Effort)

7. **Developer guide**
   - "Building Reactive Web Apps with Universal Components"
   - Pattern guide: when to use state, computed, plain values
   - Common gotchas and how to avoid them
   - Migration guide from other frameworks

---

## Examples: Before and After

### Example 1: Array Rendering

**Current (confusing):**
```chemical
#universal Notes(props) {
    state notes = []
    
    // Must remember .value for all array operations
    return {notes.value.map(note => (
        <NoteCard note={note} />
    ))}
}
```

**Improved (intuitive):**
```chemical
#universal Notes(props) {
    state notes = []
    
    // Compiler automatically handles .value for array methods
    return {notes.map(note => (
        <NoteCard note={note} />
    ))}
}
```

### Example 2: Props Passing

**Current (unclear):**
```chemical
// Parent
#universal App(props) {
    state store = { user: { name: "Alice" } }
    <Profile store={store} />  // Is store reactive or .value?
}

// Child - ambiguous
#universal Profile(props) {
    {props.store.value.user.name}  // Hope this is right
}
```

**Improved (explicit):**
```chemical
// Parent - clear that store is reactive
#universal App(props) {
    state store: Reactive<Store> = { user: { name: "Alice" } }
    <Profile store={store} />  // Type system knows it's reactive
}

// Child - clear contract
#universal Profile(props: { store: Reactive<Store> }) {
    // Compiler/IDE knows to handle .value for nested access
    {props.store.user.name}  // Works directly
}
```

### Example 3: SSR Safety

**Current (breaks on server):**
```chemical
#universal App(props) {
    // This silently fails on server
    state theme = window.localStorage.getItem('theme') || 'light'
}
```

**Improved (explicit):**
```chemical
#universal App(props) {
    // Compiler warns about window usage
    // Developer uses proper pattern
    state theme = 'light'
    if (typeof window !== 'undefined') {
        theme.value = window.localStorage.getItem('theme') || 'light'
    }
}

// OR using helper
#universal App(props) {
    // New helper automatically handles SSR
    state theme = useLocalStorage('theme', 'light')
}
```

---

## Implementation Priority

1. **Quick wins** (implement first): Auto-unwrap in array methods, better error messages
2. **Core improvements** (medium-term): Reactive type annotations, SSR helpers
3. **Advanced features** (long-term): Destructuring, more sophisticated unwrapping

---

## Conclusion

Universal components are powerful, but the reactivity model requires developers to understand internal details that shouldn't be necessary for application development. By improving compiler support, documentation, and runtime helpers, we can make #universal intuitive and delightful to use while maintaining its power and performance.

The goal is: **developers should think about *what* to build, not *how* the reactivity framework works.**
