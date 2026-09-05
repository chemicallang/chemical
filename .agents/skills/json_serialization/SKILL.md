name: json_serialization
description: Chemical JSON serialization — the `json` runtime library (JsonValue, JsonEncoder/JsonDecoder, TypeDecoder decode dispatch), the `#json(Struct)` compiler-plugin macro (`json_cbi`) that auto-generates `std::Serializer`/`std::Deserializer` impls, the std serialization interfaces in `serialization.ch`, the CBI bindings the macro relies on, and the hard-won codegen/symres pitfalls. Load before touching anything under `lang/libs/json/`, `lang/libs/json_cbi/`, or the `#json` macro.
---

# JSON Serialization (json library + json_cbi `#json` macro)

Two layers work together to make a Chemical struct serializable to/from JSON:

1. **`lang/libs/json`** — a pure-Chemical runtime library: `JsonValue`, `JsonParser`/`ASTJsonHandler`, `JsonEncoder` (writes JSON text) and `JsonDecoder` (reads a parsed `JsonValue`). It also provides the *concrete types* the macro's generated code calls (`JsonEncoder`, `JsonDecoder`, `JsonObjectDecoder`, `JsonArrayDecoder`, `TypeDecoder<T>`, `__non_gen_se_repl`).
2. **`lang/libs/json_cbi`** — a compiler plugin that implements the `#json(StructName)` top-level macro. During symbol resolution it generates, symres-links, indexes, and codegen-replaces two impls for the struct:

```chemical
impl std::Serializer<JsonValue, JsonEncoder> for Point { ... }
impl std::Deserializer<Point> for TypeDecoder<Point> { ... }
```

The **handwritten reference** for what the macro should produce lives in `lang/compiled/refmod/src/main.ch` (scratch, git-ignored): `Sample`/`SampleChild` structs with hand-written `impl std::Serializer<...> for ...` + `impl std::Deserializer<...> for TypeDecoder<...>` blocks that exercise nested structs.

---

## 1. std serialization interfaces — `lang/libs/std/src/serialization.ch`

All under `public namespace std`. These are the *interfaces*; the json library provides concrete implementations and the macro generates user impls against them.

| Interface | Purpose |
|---|---|
| `Encoder<T>` | primitive encoders (`encode_bool`, `encode_i64`, `encode_u64`, `encode_double`, `encode_float`, `encode_str`, `encode_char`, ...) + `array()`/`object()`/`map()` returning `ArrayEncoder<T>`/`ObjectEncoder<T>`/`MapEncoder<T>` |
| `Serializer<T, E : Encoder<T>>` | `func serialize(&self, encoder : &E) : std::Result<Unit, SerializationError>` — **what `#json` generates an impl of** |
| `Decoder` | primitive decoders (`decode_bool`, `decode_i64`, `decode_u64`, `decode_double`, `decode_float`, `decode_str`, `decode_char`, ...) + `array()`/`object()`/`map()` returning `std::Result<...Decoder, SerializationError>` |
| `ObjectEncoder<T>` / `ArrayEncoder<T>` / `MapEncoder<T>` | containers' write side; the generic encoders take values **by reference**: `func <V : Serializer<T>> field(&self, name : std::string_view, value : &V) : Result<Unit, SerializationError>` (same for `ArrayEncoder.encode(&self, value : &K)` / `MapEncoder.encode(&self, key : &K, value : &V)`). By-reference values mean owning/destructible members encode without copies — moving a member out of `&self` by value is illegal |
| `ObjectDecoder` / `ArrayDecoder` / `MapDecoder` | containers' read side; `ObjectDecoder.item_decoder(&mut self)` returns `Result<std::pair<std::string_view, Decoder>, SerializationError>` |
| `Deserializer<T>` | `func deserialize(&self) : std::Result<T, SerializationError>` — **what `#json` generates an impl of for `TypeDecoder<T>`** |

`SerializationError` has `kind : SerializationErrorKind` + `message : std::string`. `std::Result<T, E>` is the standard 2-arg variant result (`Ok`/`Err`).

Key consequences:
- **Encoding dispatch**: `encoder.encode<T>(value)` (generic on `JsonEncoder`, see below) requires `T : std::Serializer<JsonValue, JsonEncoder>`. The macro's generated `impl std::Serializer<JsonValue, JsonEncoder> for Point` is what makes `p` (and `p` nested inside another struct) encodable. Nested encodes go through the **generic `field<V>`** on the object encoder; nested values must therefore have their `Serializer` impl *already indexed* when the parent's generated code is instantiated — that is why a nested struct's `#json(Child)` must appear **before** `#json(Parent)` in the file.
- **Decoding dispatch**: `d.decode<T>()` (extension on `&JsonDecoder`, see below) ultimately requires `TypeDecoder<T> : std::Deserializer<T>`, which the macro provides.
- The macro **never** implements raw `std::Decoder`/`std::Encoder` — the json runtime library already does that for `JsonDecoder`/`JsonEncoder`. The macro only implements `Serializer<...>` and `Deserializer<T>`.

## 2. json runtime library — `lang/libs/json/src/`

- **`value.ch`** — `public variant JsonValue { Null(), Bool(value: bool), Number(value: std::string), String(value: std::string), Object(values : std::ordered_map<std::string, JsonValue>), Array(values : std::vector<JsonValue>) }`. Numbers are kept as raw decimal strings; decoding parses them.
- **`types.ch`** — `impl std::Serializer<JsonValue, JsonEncoder> for bool/int/.../std::string_view/std::string` (each forwards to a primitive `encoder.encode_*`) and `impl std::Deserializer<T> for TypeDecoder<T>` for each primitive (`bool`, `int`, `uint`, `double`, `float`, `string`, ...). These are **handwritten**; the macro only generates impls for **user structs**, and primitive fields are encoded/decoded through `JsonEncoder`/`JsonDecoder` primitives directly.
- **`encode.ch`** — `JsonEncoder { buffer : *mut std::string, counts : *mut std::vector<u64> }` (raw pointer members: only construct via `JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }`). Contains `impl std::Encoder<JsonValue> for JsonEncoder` (writes into `buffer`), `JsonObjectEncoder`/`JsonArrayEncoder`/`JsonMapEncoder` with the container impls, the single string escaper `json_escape_into` (+ `append_escaped_char`), and the generic entry point:

```chemical
func <T : std::Serializer<JsonValue, JsonEncoder>> (e : &JsonEncoder) encode(value : T) : std::Result<std::Unit, std::SerializationError> {
    // casts &JsonEncoder to &std::Encoder<JsonValue> via __unsafe_cast_json_encoder
    return value.serialize(__unsafe_cast_json_encoder<JsonValue>(e)) as std::Result<std::Unit, std::SerializationError>
}
```

- **`decode.ch`** — decoder side:
  - `JsonDecoder { value : &JsonValue }` + `impl std::Decoder for JsonDecoder` (every primitive `decode_*`, plus `object()` → `JsonObjectDecoder`, `array()` → `JsonArrayDecoder`, `map()` → `JsonMapDecoder`).
  - `JsonObjectDecoder { iterator : std::ordered_map_iterator<std::string, JsonValue>, _total : u64 }` with `item_decoder(&mut self)` returning `Result<std::pair<std::string_view, JsonDecoder>, SerializationError>`; each call advances the iterator and hands back the next key + a **fresh `JsonDecoder` for that field's value**.
  - `TypeDecoder<T> { var decoder : &JsonDecoder }` — a placeholder type that exists only so `Deserializer<T>` impls can be attached per `T`.
  - Dispatch chain: `func <T> (decoder : &JsonDecoder) decode() : Result<T, SerializationError>` builds `TypeDecoder<T>` and routes through `decode_it_2<T, TypeDecoder<T>>` → `decode_it_1<T, K : Deserializer<T>>` → `k.deserialize()` (interface dispatch). This is why adding a new `Deserializer<T>` impl (via `#json`) makes `d.decode<T>()` work without touching the runtime.
  - `__non_gen_se_repl(value : &mut std::SerializationError, repl : std::SerializationError) : std::SerializationError` — **non-generic** helper that stores `repl` into `value` and returns the original error. The `#json` macro calls it from generated deserialize error paths instead of generic `std::replace` (see pitfalls).
  - `take_ok<T>(t : &mut std::Result<T, std::SerializationError>) : T` — mirrors `std::Option.take`: memcpy's the **Ok payload out** of the `Result` and re-initializes the slot to an `Err` state (bitwise, no payload destructor runs), returning an **owned local**. The `#json` macro's generated deserialize uses it for composite/string fields: a pattern-bound Ok payload cannot be moved onward into an aggregate literal ("cannot move this value without re-initializing memory"), but the taken local can — no copies, no double-destroy of the dead `Result`. Inside the generic body the `std::SerializationError` literal must be `std::`-qualified (unqualified names fail to resolve during generic instantiation).
- **`handler.ch`** — SAX handlers (`ASTJsonHandler` builds a `JsonValue` AST; `DebugJsonSaxHandler` prints).
- **`json.ch`** — `JsonParser(bufferSize, ...)` → `.parse(text, len, &mut handler)` returning `ParseResult { ok, ... }`. `parse_string_value`/`parse_string_key` free the growable heap buffer on **every** path (including parse errors).
- **`emit.ch`** — string-based JSON emission: `(output : &mut std::string) append_value(value)` / `append_value_pretty(value, indent)` share one `append_value_inner` (pretty flag + indent). The old `JsonStringEmitter` interface / `JsonStringBuilder` / `JsonStringPrinter` / `escape_string_into` were removed — escaping is the single `json_escape_into` in encode.ch.

### High-level API — `api.ch` (`public namespace json`)

The beginner entry points (called as `json::parse(...)` etc.; the pre-existing types stay module-top-level and unqualified):

```chemical
var r = json::parse(text)                    // Result<JsonValue, JsonParseError> (JsonParseError { pos, message })
var s = json::stringify(&value)              // std::string (compact)
var s = json::stringify_pretty(&value)       // std::string (indented)
var e = json::encode<Point>(&point)          // Result<std::string, SerializationError> (by-ref, no copies)
var d = json::decode<Point>(&value)          // Result<Point, SerializationError>
var p = json::decode_str<Point>(text)        // parse + typed decode in one call
```

Implementation notes / compiler quirks:
- `encode<T>` / `decode<T>` are **public generic** functions, so every callee they reach must be public too (retention rule): `__unsafe_cast_json_encoder`, `se_err`, `__decode_dispatch_1/2`, the `decode` extension method are `public` even though `__`-prefixed.
- `&raw mut` of a generic-instantiation local (`std::vector<u64>`) inside a generic function hits a type-alias quirk — the encoder state is created in non-generic helpers `__new_counts()` / `__new_encoder(&mut output, &mut counts)`.
- Extracting the `Err` payload of a `Result` inside a generic function leaks the generic `E` and cannot be passed to non-generic code (pattern-match `var Err(e) = r` inside `json::encode<T>` fails). The high-level `encode<T>` therefore returns a **static** error message on failure; the low-level container API (`ObjectEncoder.field` etc.) still propagates real errors.
- `decode_str<T>` is generic, so its `Result<JsonValue, JsonParseError>` pattern binds would mis-type in the generic context — the parse + payload extraction lives in the **non-generic** `__try_parse(text) : std::Option<JsonValue>` (module-level, `None` on parse error), and the generic body takes the payload out with `std::Option.take()` and borrows it for `d.decode<T>()`. Parse-error detail is dropped (static "json parse error" message).

### Canonical round-trip usage (what tests do)

```chemical
// beginner path (new)
var e = json::encode<Point>(&point)          // Result<string, SerializationError>
var Ok(text) = e else unreachable
var r = json::parse(text.to_view())          // Result<JsonValue, JsonParseError>
var Ok(value) = r else unreachable
var d = json::decode<Point>(&value)          // Result<Point, SerializationError>

// low-level path (advanced / manual)
var output = std::string()
var counts = std::vector<u64>()
var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
var e = encoder.encode<Point>(Point { x : 10, y : 20 })   // or p.serialize(&encoder)
// output now contains {"x":10,"y":20}

var ph = ASTJsonHandler()
var parser = JsonParser(128, 4096)
var r = parser.parse(output.data(), output.size(), &mut ph)   // r.ok

var d = JsonDecoder { value : &ph.root }
var res = d.decode<Point>()                                   // Result<Point, SerializationError>
var Ok(v) = res else unreachable
```

## 3. `#json(Struct)` macro — `lang/libs/json_cbi/`

Module `json_cbi` (`chemical.mod`: `source "src"`, imports `cstd`, `std`, `compiler`). Two source files:

- `src/types.ch` — `SerializableInfo` (per-macro-node state; everything the codegen phases need, incl. pre-instantiated generic types) + the three no-op callback stubs (`known_type_fn`, `child_res_fn`, `cross_mod_sym_decl_proxy_fn`) required by `make_top_level_embedded_node`.
- `src/main.ch` — the actual plugin: parse hook, symres link-body hook, codegen replacement hooks, and the AST builders.

### Registration & phases

The macro is wired through `build.lab`/`chemical.mod` in `lang/libs/json_cbi/` (registered as CBI `"json"` with a marker annotation + `build_cbi`), following the same shape as the other `*_cbi` plugins. A `#json(Point)` occurrence parses to an **`EmbeddedNode`** carrying a `SerializableInfo` in its data pointer. Phase hooks:

| Hook (`@no_mangle public func`) | Role |
|---|---|
| `json_parseMacroTopLevelNode(parser, builder, spec)` | parse `#json(<type>)`, join the path segments (`Type` or `ns::Type` / `a::b::Type`) into one `::`-separated `struct_name`, allocate `SerializableInfo`, return `make_top_level_embedded_node(...)` with the 3 no-op callback fns |
| `json_symResDeclareNode` / `json_symResLinkSigNode` | intentionally **empty** — everything happens in the link-body phase |
| `json_symResNode(visitor, node)` | **does all the work** (see below) |
| `json_replacementNodeDeclare(builder, node)` | returns `info.replacement_scope` (used by the 2c **declare** pass so prototypes/vtables are emitted) |
| `json_replacementNode(builder, node)` | returns the **same** `info.replacement_scope` (2c body pass emits the definitions) |

### `json_symResNode` — step by step

1. **Resolve the target struct** via `resolve_type_path(resolver, &struct_name)` — the first path segment through `resolver.resolve` (a namespace resolves to its root scope), remaining segments through `node.child(name)` walking; supports `#json(Point)` and `#json(ns::Point)`. Then snapshot its members: `field_names` (allocated views), `field_types` (`*mut BaseType` per member).
2. **`resolve_types(resolver, info, loc)`** — resolve every symbol the generated code will reference and cache the AST nodes on `info`:
   - std nodes: `std::Serializer`, `std::Deserializer`, `std::Result` + `Ok`/`Err` members, `std::Unit`, `std::SerializationError`, `std::Encoder.object`, `std::ObjectEncoder.field`, `std::pair.second`. (No `std::replace` — dead since generated error paths use `__non_gen_se_repl`.)
   - json-lib nodes: `JsonEncoder`, `JsonValue`, `JsonDecoder` + its `decode_i64/u64/double/float/str/bool/char`, `object`, and the generic `decode` extension; `JsonObjectDecoder.item_decoder`; `TypeDecoder`; and `__non_gen_se_repl`.
   - Nodes are found with `std_node.child(name)` / `resolver.resolve(name)`; impl-contained methods (e.g. `JsonDecoder.decode_i64`, which lives in `impl std::Decoder for JsonDecoder`) **are** discoverable through `child()` on the struct node.
3. **Build + visit the generic types** (`visitor.visitType(...)`), caching them on `info` for codegen reuse: `Serializer<JsonValue, JsonEncoder>`, `Result<Struct, SE>`, `Result<Unit, SE>` (fresh LinkedTypes — see pitfalls), `Deserializer<Struct>`, `TypeDecoder<Struct>`, and per-struct-field `Result<field_type, SE>` for fields that decode via `decode<T>` (`info.field_result_types`).
4. **Build the replacement tree**: one `Scope` (`make_scope`) containing two `ImplDefinition`s (`make_impl_def`) — `impl Serializer<JsonValue,JsonEncoder> for Point` and `impl Deserializer<Point> for TypeDecoder<Point>`.
5. **`build_serialize_fn`** / **`build_deserialize_fn`** create the contained functions with `make_function` + `make_function_param` + `add_body`/`body.push`, then `ser_impl.add_function(...)`/`deser_impl.add_function(...)` (binding `ImplDefinitionadd_function`).
6. **`resolver.index_impl(impl)`** — mirrors what the module index phase does for parsed impls: registers the impl in the interface's implementations index and **adopts** its contained functions into the struct/type member tables (so `point.serialize(&e)` and interface dispatch `decode<T>` can find them). Without this, method calls resolve to the bare interface name and links fail.
7. **`visitor.visitNode(ser_impl)` / `visitor.visitNode(deser_impl)`** — symres-link the generated functions **right here, during the macro's link-body phase** (all identifiers are pre-linked; chain members are re-resolved by symres against concrete receiver types).
8. Store `info.replacement_scope = impl_scope` for the codegen hooks.

### Generated `serialize` (target shape)

```chemical
impl std::Serializer<JsonValue, JsonEncoder> for Point {
    func serialize(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        var obj = encoder.object()                       // JsonObjectEncoder
        obj.field<int>("x", &self.x)                     // one per field; T = concrete field type
        obj.field<int>("y", &self.y)
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit())
    }
}
```

- `self` is `&Point`; `encoder` is `&JsonEncoder`.
- Field calls are emitted as **`AccessChainNode` statements** built from the method **chain** (`make_function_call_node(chain, ...)`), never by wrapping a pre-built `FunctionCall` (that would nest a second call on the first call's result). The explicit generic arg (`field_stmt.add_generic_arg(field_type, loc)`) is added on the statement; args are the field-name `StringValue` and a `&self.<field>` reference (`make_reference_of_value` over the member chain, matching the `value : &V` field signature).
- Known TODO: unlike the handwritten reference, the generated serialize currently does **not** inspect each `field(...)` result for `Err` — it always returns `Ok(Unit())`. Error propagation on the encode side is unfinished work.

### Generated `deserialize` (target shape)

```chemical
impl std::Deserializer<Point> for TypeDecoder<Point> {
    func deserialize(&self) : std::Result<Point, std::SerializationError> {
        var __dr = self.decoder.object()                          // Result<JsonObjectDecoder, SE>
        if(__dr is std::Result.Err) { ...Err-return... }
        var Ok(__obj) = __dr else unreachable
        // per field, in struct order:
        var __q0 = __obj.item_decoder()                           // consume one key/value pair
        if(__q0 is std::Result.Err) { ...Err-return... }
        var Ok(__p0) = __q0 else unreachable                      // pair<string_view, JsonDecoder>
        var __v0 = __p0.second.decode_i64()                       // scalar → decode_XXX
        if(__v0 is std::Result.Err) { ...Err-return... }
        var Ok(__f0) = __v0 else unreachable
        // composite fields (nested structs + std::string/string_view) instead:
        var __v1 = __p1.second.decode<T>()                        // decode<T> → Result<T, SE>
        if(__v1 is std::Result.Err) { ...Err-return... }
        var __f1 : T = take_ok(&mut __v1)                         // move payload out (owned local)
        ...
        return std::Result.Ok<Point, std::SerializationError>(Point { x : __f0, child : __f1 })
    }
}
```

- `self` is `&TypeDecoder<Point>`; `self.decoder` is the member `&JsonDecoder`.
- **Flat, negative-check-first structure** (early returns), exactly like the handwritten reference — there is no `if Ok { nest next field }` nesting.
- `append_err_check(...)` is a shared helper that emits the `if(<var> is std::Result.Err) { var Err(__e) = <var> else unreachable; return std::Result.Err<T, SE>(__non_gen_se_repl(&mut __e, SerializationError())) }` block.
- Scalar fields: `find_decoder_method` maps the field's `BaseType` to a `JsonDecoder.decode_*` method node (ints/longs → `decode_i64` + a cast in the struct init; uints → `decode_u64`; **`char`/`uchar` → `decode_char`** (chars encode as single-char JSON strings, not numbers — routing them to `decode_u64` would reject the encoded form); float/double/bool → their methods; note **`std::string`/`string_view` are NOT routed to `decode_str`** — they return null here so they decode through the generic path below).
- Composite fields (nested structs + strings): no scalar method → build the chain `__p<i>.second.decode<T>()` with an explicit generic arg = the field type, then **`take_ok(&mut __v<i>)`** moves the Ok payload out (the `Result` is left in an `Err` state and its destructor won't touch the payload). A pattern-bound Ok payload cannot be moved onward into an aggregate literal; the taken owned local can. See pitfalls for the **explicit result var type** requirement.
- Struct construction: `make_struct_value(struct_node)` + `add_value(field_name, ...)` where scalar fields are wrapped in `casted_value(pattern-var-id, field_type)` but **composite fields are added as plain identifiers** — a `CastedValue` around a destructible value makes codegen bitwise-copy it into the member and then still destroy the local at scope end (double free).
- Per-field temp names are formatted with `snprintf` into allocator buffers (`alloc_indexed_view(builder, "__q%d", i)`).

## 4. CBI bindings the macro depends on

The plugin needed several new compiler-CBI bindings. All live in `compiler/cbi/bindings/` (+ the Chemical wrappers in `lang/libs/compiler/src/*.ch` and symbol registration in `compiler/cbi/bindings/CBI.cpp`). If a builder API is missing for a future macro, these are the templates to copy:

| Binding | File | Purpose |
|---|---|---|
| `ASTBuildermake_generic_type_with_args` | ASTBuilderCBI.cpp | `GenericType` from a linked decl + `(type, loc)` argument spans |
| `ASTBuildermake_reference_of_value` | ASTBuilderCBI.cpp | `&mut`-style reference value (mirrors parser `&mut`) |
| `ASTBuildermake_pattern_match_expr` / `make_pattern_match_node` / `PatternMatchExprset_expression` / `set_else_unreachable` / `add_param_name` | ASTBuilderCBI.cpp | build `var Ok(x) = expr else unreachable` statements |
| `FunctionCalladd_generic_arg` / `FunctionCallNodeadd_generic_arg` | ASTBuilderCBI.cpp | explicit generic args on calls / call statements |
| `ASTBuildermake_identifier` (null-safe) | ASTBuilderCBI.cpp | tolerate `null` links; skip `known_type()` for `PatternMatchId` |
| `ASTBuildermake_function` (sets `signature_resolved`) | ASTBuilderCBI.cpp | auto-created fn decls count as signature-resolved |
| `ImplDefinitionadd_function` / `StructDefinitionadd_function` | ASTBuilderCBI.cpp | attach contained funcs to impls/structs |
| `SymResLinkBodyvisitType` | SymbolResolverCBI.cpp | `visitor.visitType(BaseType*, loc)` from plugins (instantiation trigger) |
| `SymbolResolverindex_impl` | SymbolResolverCBI.cpp + LinkSignature.cpp wrapper | register + adopt a macro-generated impl (see above) |
| `ParserparseType` | ParserCBI.* | parse a type expression from the macro parser (used by future macros) |
| `EmbeddedNode` replacement callbacks (`known_type_fn`, `child_res_fn`, `cross_mod_sym_decl_proxy_fn`) | existing EmbeddedNode machinery | required params of `make_top_level_embedded_node`; the json macro passes no-ops |
| `CTopLevelDeclarationVisitor::VisitScope` | preprocess/2c/2cASTVisitor.cpp + CTopLevelDeclVisitor.h | **declare pass** must recurse into the replacement `Scope` (macro-generated impls otherwise get bodies but **no C prototypes** → cross-module ordering errors in the generated C) |

Also note the symres change in `compiler/symres/SymResLinkBody.cpp`: `VisitVariableIdentifier` honors a **pre-set `identifier->linked`** (plugin-generated identifiers) instead of only resolving by name — chain members are still re-resolved by `find_link_in_parent` against concrete receiver types.

## 5. Gotchas & battle scars (read before changing anything)

1. **`GenericType::instantiate()` mutates `referenced->linked`.** Two different generic types built from the *same* `LinkedType` base (e.g. `Result<Point, SE>` and `Result<Unit, SE>`) alias each other after instantiation. Always build **fresh** `make_linked_type` instances per generic type (`result_linked`/`result_linked2`, `se_linked`/`se_linked2`, ...).
2. **Pre-link every identifier to a real node; never pass `null` blindly.** Plugin identifiers are visited by symres outside normal name resolution; identifiers with no link crash later (notably function-call "parent" identifiers like `Ok(...)`/`Err(...)`/`replace(...)`/`SerializationError()`, which must link to the resolved `VariantMember`/function decl). The one safe exception: identifiers that will become **access-chain members** (positions ≥ 1 in a chain) are unconditionally re-linked by `find_link_in_parent` against the receiver's concrete type.
3. **Method-call statements must be `AccessChainNode`s over the chain, not `ValueNode`s.** `make_function_call_node(chain, ...)` mirrors the parser's bare-call statement and terminates with `;` + destructor handling; a `make_value_node` wrapping a call emits no `;` and produces invalid C (`...})) (*({...})` parses as a call). **Never** pass a pre-built `FunctionCall` as the chain (the binding wraps it in a second call whose parent is the first call's result).
4. **`&mut` must be `ReferenceOfValue`, not `AddrOfValue`** — `AddrOfValue` (`&raw mut`) routes through an extra temp during call codegen and emits an extra `&`.
5. **Generic `std::replace` from generated code corrupts under nested instantiation.** If a generated (non-generic) function calls `std::replace<T>` *after* a statement that triggered a generic function instantiation (e.g. nested `decode<T>`), the replace call can register a bogus instantiation with an unspecialized generic arg → `[GENERIC_TYPE_PARAMETER_NOT_SPECIALIZED_COMPILER_BUG]` in the generated C. Fix: use the **non-generic** `__non_gen_se_repl` (in `lang/libs/json/src/decode.ch`); the handwritten reference does the same on purpose.
6. **`decode<T>()` calls leave the generic *master* `Result` type on later statements.** While the `decode<Child>` instantiation finalizes (asynchronously), statements after the call (the `Ok/Err` pattern-match temps) stay typed as the un-instantiated `std::Result` → the pattern-match pointer temp emits as `std_stdResult*` (undeclared C symbol). Fix: pre-instantiate `Result<field_type, SE>` per struct-typed field and give the decode-result `var` an **explicit concrete type** (`make_varinit_stmt(..., type, ...)`); identifiers linked to that var then inherit the concrete type at creation.
7. **Pattern-match node move bug**: `make_pattern_match_node` moves the configured `PatternMatchExpr` into the node's embedded expression. After the move, each `PatternMatchIdentifier`'s `matchExpr` must be **re-pointed at `&node->value`** (done in the binding) or codegen's `local_allocated` lookup fails and destructured names emit as bare C identifiers.
8. **Contained functions must have the `ImplDefinition` as AST parent**, so the mangler emits the impl-context name (`std_stdSerializer__cgs__0_Point_serialize`) that vtable generation expects — exactly like parsed `impl` code. Building the fn under the struct decl instead yields `main_Pointserialize` and the vtable/interface links break.
9. **Macro impls are not indexed automatically.** Parsed impls get indexed by the module's index phase (`index_implementation` → `container->adopt(impl)` + `implsIndex.add_interface`). Macro-generated impls must be indexed explicitly via `resolver.index_impl` or `p.serialize(...)`/`decode<T>()` resolution fails.
10. **The 2c declare pass must see the replacement `Scope`.** Without `CTopLevelDeclarationVisitor::VisitScope` recursing into replacement nodes' scopes, generated impls get definitions but no C prototypes → `error: declaration expected` / implicit-declaration clashes in `Translated.c` when json-module instantiated code calls them.
11. **Field decode order = JSON key order = struct member order.** The decoder consumes pairs by position (`item_decoder()` per field) and never compares the key name — matches the handwritten reference. Keep struct member order stable between encode and decode.
12. **Unsupported field shapes get a clean `#json`-site error.** `json_symResNode` validates each member type (`unsupported_field_reason`) and calls `resolver.error` + aborts that `#json` for shapes that clearly can't serialize: arrays, pointers/references, unions, `Linked`→`VariantDecl`/`UnionDecl`, dynamic/function/void kinds. `std::string`/`string_view`, nested structs (own `#json`/impl) and **enum fields** are supported; **`resolver.error` must receive a static/literal `string_view`** (a view into a stack-local `std::string` dangles and the error is silently dropped — the field then vanished with no diagnostic, fixed by returning `string_view` literals from the reason helper).
13. **Enum fields serialize as the member NAME string.** A `#json` struct member of an `enum` type encodes as `"Red"` (if/else chain over `(self.field as int) == <ordinal>` assigning `std::string("Red")`, then `obj.field<std::string>("name", &__en<i>)`) and decodes by hashing the decoded name with `std::fnv1_hash_view` and dispatching over **precomputed name hashes** (`fnv1a64` computed in the plugin at macro time, compared via `make_ubigint_value` cast to the fn's return type) back to `0 as EnumType`-style values; an unknown name returns `Err<Struct, SE>(se_err(...))`. Impls on enums are impossible (the compiler rejects `impl X for SomeEnum`), so this is macro per-field codegen only.
14. **`#json` does NOT support `vector`/`map`/`Option` fields yet.** Struct members typed with a generic container still break the json module's own symres (`unresolved child 'serialize' in parent 'value'` cascade) rather than producing the clean error above — by the time the macro reads member types the container has been monomorphized into an ordinary-looking decl that the `BaseTypeKind`/linked-node-kind check can't distinguish from a plain struct (verified empirically: the member-kind check returns "supported"). Needs container-name/decl-origin detection or `Serializer`/`Deserializer` impls for the containers. Variants are likewise not yet supported (see the follow-up: `#json(Variant)` with case-name-object wire format needs case-param introspection through the CBI).
15. **Destructible/owning members are supported end-to-end** (owning strings, nested structs with `@delete`, structs whose members own resources). Encode passes `&self.<field>` by reference (never moves/copies members out of `&self`); decode moves payloads out of their `Result`s with `take_ok` and adds composite members to the final literal **without** a `CastedValue` — a cast around a destructible value bitwise-copies it into the member and the source local is still destructed at scope end (double free). Always verify a destructible round trip with destructor-count checks.
16. **Public-generic retention rule.** Public generic functions (the container impl methods, `json::encode<T>`, the `decode` dispatch chain) may only call **public** functions — internal helpers hit "calling a non-retained function in a public generic declaration". The `__`-prefixed helpers (`__unsafe_cast_json_encoder`, `se_err`, `__decode_dispatch_1/2`, `__container_begin_item`) are public for exactly this reason.
17. **Float/double precision.** `write_double_raw` uses `%.17g` and `write_float_raw` uses `%.9g` — the exact round-trip precisions for IEEE-754 double/float. The old `%.6g` silently corrupted values like `0.1+0.2` (emitted `0.3`). Expect non-pretty float text like `3.1400001` for `3.14f`.
18. **`encode_char` escapes.** A `char` is a single-char JSON string and the char itself is escaped (quotes/backslashes/control chars) via `json_escape_into` over a 1-char buffer — `'"'` emits `"\""`.
19. **One container struct cannot implement all three encoder interfaces.** A single `JsonContainerEncoder` implementing `ArrayEncoder`/`ObjectEncoder`/`MapEncoder` breaks interface-constraint resolution for the two-generic-param `MapEncoder.encode<K, V>` (unresolved `serialize` on the `V` param). The three structs stay separate; the shared comma/bookkeeping is the `__container_begin_item` free function.

## 6. Testing & debugging

Scratch packages live under `lang/compiled/` (git-ignored) so they never pollute the repo:

- `lang/compiled/temp/chemical.mod` — single-file test app. Its `source "src/<file>.ch"` line selects the active test:
  - `json2.ch` — `#json(Point)` scalar round trip (`ROUND TRIP OK`).
  - `json3.ch` — `#json(Child)` then `#json(Parent)` nested round trip (`NESTED ROUND TRIP OK`). Requires Child's `#json` above Parent's.
- `lang/compiled/refmod/chemical.mod` — the **handwritten reference** (`Sample`/`SampleChild` impls by hand; no `#json`). Compiles/runs the same checks and validates the runtime library + bindings without the macro.
- `lang/tests/src/libs/json/` — committed json runtime tests (`main.ch` parser/encoder/decoder + high-level API; `struct_tests.ch` handwritten impls + `json::encode<T>`/`json::decode<T>`; `utils.ch` helper). Run with `./lang/tests/build/tests-tcc.exe --test-names <comma-separated-func-names>`.
- `lang/tests/compiler_plugins/json/src/test.ch` — committed `#json` macro tests (scalar/nested/wide structs, error paths, destructible round trips with destructor counters, `#json(jmns::JMNs)` namespaced, char/uchar fields). Built via `./cmake-build-debug/TCCCompiler lang/tests/build.lab -o <exe> --mode debug_complete -arg test-plugins` (the `import "./compiler_plugins/json/build.lab"` is currently dangling — the plugin module links only if a matching build.lab exists; the test module's `@test` fns can also be linked directly from a scratch app importing it + a `test_runner(argc, argv)` main).

Compile + run a scratch package:

```bash
cmake-build-debug/TCCCompiler lang/compiled/temp/chemical.mod -o lang/compiled/temp/temp.exe -v -bm-modules
./lang/compiled/temp/temp.exe
```

After touching compiler C++: `./scripts/build.sh --tcc`. Full regression: `./scripts/test.sh --tcc` (expect `Total <N> Passed <N> Failed 0`).

Generated C lands at `lang/compiled/temp/build/Translated.c`. Useful search anchors when the C is wrong:

- `std_stdSerializer__cgs__0_<Struct>_serialize` / `std_stdDeserializer__cgs__<N>_TypeDecoder__cgs__<N>_deserialize` — the macro-generated functions.
- `std_stdDecoder_JsonDecoder_object`, `std_stdObjectDecoder_JsonObjectDecoder_item_decoder`, `json_JsonDecoderdecode__cfg_<N>`, `json___non_gen_se_repl` — runtime calls the generated code must resolve to.
- `[GENERIC_TYPE_PARAMETER_NOT_SPECIALIZED_COMPILER_BUG]` — pitfall #5/#6 happened.
- `std_stdResult*` without a `__cgs__` suffix — pitfall #6 happened (pattern temp typed at the generic master).

Debugging loop: fix → rebuild plugin-only changes need no C++ rebuild (the plugin `.ch` is JIT-compiled at runtime) → recompile scratch package → inspect `Translated.c`/run exe.
