# Generic std-container JSON serialization

**Chemical · Proposal 0014**

> Give the generic standard containers serializer/deserializer impls so `#json`
> works for `vector<T>`, `unordered_map<K,V>` and `ordered_map<K,V>`.

**Status:** Approved · **Area:** Libraries · JSON · [review index](review.md)

## Goal fit

- **R3** ecosystem (JSON works with std types)
- **R0** recursive, zero-overhead dispatch

## 1. Problem

The `#json(Struct)` macro auto-generates `std::Serializer`/`std::Deserializer`
impls and the runtime decodes through `TypeDecoder<T>` dispatch. But
`std::vector<T>`, `std::unordered_map<K,V>` and `std::ordered_map<K,V>` are
**generic declarations with no such impls**, so a struct containing them cannot be
serialized — the plugin has nothing to dispatch to. Writing a separate impl per
element type is the only current workaround, which does not scale.

## 2. Goals & non-goals

- **In scope:** generic impls of the serializer/deserializer traits for vectors and
  the two maps (and the corresponding `TypeDecoder` dispatch), recursing into
  element/key/value types; arrays; `Option`/`Result` as a follow-on if cheap.
- **Out of scope:** new JSON features; a redesign of the `#json` macro; binary
  formats.

## 3. Proposed approach

1. **Prerequisite.** Depends entirely on proposal 0003 (generic impls). Until then
   this is not expressible.
2. **Vector → JSON array.** `impl<T> Serializer for vector<T> where T: Serialize`
   (bound spelled per the chosen bounds syntax): emit `[`, serialize each element,
   `]`. Decode mirrors it, requiring `T: Deserialize`.
3. **Maps → JSON object.** `impl<K,V> Serializer for unordered_map<K,V>` and
   `ordered_map<K,V>`: keys must be string-serializable (restrict `K` to
   string-like via a bound); values recurse. `ordered_map` preserves insertion
   order in the output; `unordered_map` order is unspecified.
4. **Recursive dispatch.** Element serialization goes through the same trait so
   nested containers (`vector<vector<int>>`, maps of vectors) work automatically.
5. **Strings & keys.** Reuse the existing string serializer; ensure JSON escaping
   is shared, not reimplemented.
6. **Diagnostics.** A container of a non-serializable element type produces a clear
   "`T` does not implement Serialize" error at the use site.

## 4. Alternatives considered

- **Per-element-type impls in user code**: rejected — combinatorial and
  unmaintainable.
- **Macro-generate impls**: rejected — macros cannot express the open-ended
  recursion; the language trait system should carry this.

## 5. Phased milestones

1. Implement `vector<T>` serializer/deserializer + decoder dispatch.
2. Implement `unordered_map<K,V>` and `ordered_map<K,V>`.
3. Recursive/nested containers; arrays; `Option`/`Result`.
4. Round-trip tests through `#json`; negative tests for non-serializable element
   types.
5. Run the common core-language + libs tests on interpreter, TCC and LLVM.

## 6. Acceptance criteria

- A struct with `vector<int>`, `unordered_map<string,int>` and nested containers
  round-trips through `#json` on TCC and LLVM.
- Non-serializable element types fail with an actionable diagnostic.
- No meaningful serialization overhead vs. hand-written impls.
- Tests run on interpreter, TCC and LLVM.

## 7. Risks & open questions

- How map keys that are not strings are handled (numbers? error?).
- Trait surface: does the existing `std::Serializer`/`TypeDecoder` design
  accommodate this directly, or does it need a small extension?
- Interaction with bounds syntax chosen in 0003.

## 8. Dependencies

Depends on: 0003 (generic impls) · Unblocks: usable `#json` for real data models.

## 9. Evidence & references

- AGENTS.md: `#json(Struct)` generates `std::Serializer`/`std::Deserializer`;
  `TypeDecoder<T>` decode dispatch.
- `lang/libs/json/`, `lang/libs/json_cbi/`;
  `.agents/skills/json_serialization/SKILL.md`.
- `lang/docs/library-ecosystem-analysis.md` (JSON ↔ http wiring etc.).
- `lang/docs/proposals/DECISIONS.md` (spawned from decision 03).
