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
| `ObjectEncoder<T>` / `ArrayEncoder<T>` / `MapEncoder<T>` | containers' write side; `ObjectEncoder<T>` has the generic `func <V : Serializer<T>> field(&self, name : std::string_view, value : V) : Result<Unit, SerializationError>` |
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
- **`encode.ch`** — `JsonEncoder { buffer : *mut std::string, counts : *mut std::vector<u64> }` (raw pointer members: only construct via `JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }`). Contains `impl std::Encoder<JsonValue> for JsonEncoder` (writes into `buffer`), `JsonObjectEncoder`/`JsonArrayEncoder`/`JsonMapEncoder` with the container impls, and the generic entry point:

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
- **`handler.ch`** — SAX handlers (`ASTJsonHandler` builds a `JsonValue` AST; `DebugJsonSaxHandler` prints).
- **`json.ch`** — `JsonParser(bufferSize, ...)` → `.parse(text, len, &mut handler)` returning `ParseResult { ok, ... }`.

### Canonical round-trip usage (what tests do)

```chemical
// encode
var output = std::string()
var counts = std::vector<u64>()
var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
var e = encoder.encode<Point>(Point { x : 10, y : 20 })   // or p.serialize(&encoder)
// output now contains {"x":10,"y":20}

// parse text into a JsonValue
var ph = ASTJsonHandler()
var parser = JsonParser(128, 4096)
var r = parser.parse(output.data(), output.size(), &mut ph)   // r.ok

// decode back
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
| `json_parseMacroTopLevelNode(parser, builder, spec)` | parse `#json(<name>)`, store `struct_name`, allocate `SerializableInfo`, return `make_top_level_embedded_node(...)` with the 3 no-op callback fns |
| `json_symResDeclareNode` / `json_symResLinkSigNode` | intentionally **empty** — everything happens in the link-body phase |
| `json_symResNode(visitor, node)` | **does all the work** (see below) |
| `json_replacementNodeDeclare(builder, node)` | returns `info.replacement_scope` (used by the 2c **declare** pass so prototypes/vtables are emitted) |
| `json_replacementNode(builder, node)` | returns the **same** `info.replacement_scope` (2c body pass emits the definitions) |

### `json_symResNode` — step by step

1. **Resolve the target struct** (`resolver.resolve(struct_name)`) and snapshot its members: `field_names` (allocated views), `field_types` (`*mut BaseType` per member).
2. **`resolve_types(resolver, info, loc)`** — resolve every symbol the generated code will reference and cache the AST nodes on `info`:
   - std nodes: `std::Serializer`, `std::Deserializer`, `std::Result` + `Ok`/`Err` members, `std::Unit`, `std::SerializationError`, `std::Encoder.object`, `std::ObjectEncoder.field`, `std::pair.second`, `std::replace`.
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
        obj.field<int>("x", self.x)                      // one per field; T = concrete field type
        obj.field<int>("y", self.y)
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit())
    }
}
```

- `self` is `&Point`; `encoder` is `&JsonEncoder`.
- Field calls are emitted as **`AccessChainNode` statements** built from the method **chain** (`make_function_call_node(chain, ...)`), never by wrapping a pre-built `FunctionCall` (that would nest a second call on the first call's result). The explicit generic arg (`field_stmt.add_generic_arg(field_type, loc)`) is added on the statement; args are the field-name `StringValue` and a `self.<field>` chain.
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
        var __v0 = __p0.second.decode_i64()                       // scalar → decode_XXX; nested struct → decode<T>()
        if(__v0 is std::Result.Err) { ...Err-return... }
        var Ok(__f0) = __v0 else unreachable
        ...
        return std::Result.Ok<Point, std::SerializationError>(Point { x : __f0, y : __f1 })
    }
}
```

- `self` is `&TypeDecoder<Point>`; `self.decoder` is the member `&JsonDecoder`.
- **Flat, negative-check-first structure** (early returns), exactly like the handwritten reference — there is no `if Ok { nest next field }` nesting.
- `append_err_check(...)` is a shared helper that emits the `if(<var> is std::Result.Err) { var Err(__e) = <var> else unreachable; return std::Result.Err<T, SE>(__non_gen_se_repl(&mut __e, SerializationError())) }` block.
- Scalar fields: `find_decoder_method` maps the field's `BaseType` to a `JsonDecoder.decode_*` method node (ints/longs → `decode_i64` + a cast in the struct init; uints/char → `decode_u64`; float/double/bool/string → their methods).
- Nested struct fields: no scalar method → build the chain `__p<i>.second.decode<T>()` with an explicit generic arg = the field type. See pitfalls for the **explicit result var type** requirement.
- Struct construction: `make_struct_value(struct_node)` + `add_value(field_name, casted_value(pattern-var-id, field_type))`, then `Result.Ok<T, SE>(struct)`.
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
12. **Unsupported/edge field types** (arrays, maps, custom primitives, `std::string` decode cast) currently skip their decode slot or rely on `decode<T>` — extend `find_decoder_method` + the runtime's `TypeDecoder<T>` primitive impls when adding support.

## 6. Testing & debugging

Scratch packages live under `lang/compiled/` (git-ignored) so they never pollute the repo:

- `lang/compiled/temp/chemical.mod` — single-file test app. Its `source "src/<file>.ch"` line selects the active test:
  - `json2.ch` — `#json(Point)` scalar round trip (`ROUND TRIP OK`).
  - `json3.ch` — `#json(Child)` then `#json(Parent)` nested round trip (`NESTED ROUND TRIP OK`). Requires Child's `#json` above Parent's.
- `lang/compiled/refmod/chemical.mod` — the **handwritten reference** (`Sample`/`SampleChild` impls by hand; no `#json`). Compiles/runs the same checks and validates the runtime library + bindings without the macro.
- `lang/tests/src/libs/json/` — committed json runtime tests (run inside the standard compiled suite).

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
