# Web/DOM type declarations

**Chemical · Proposal 0008**

> A first-party `dom` module shipped with the compiler, so browser code and
> universal components are typed and checked instead of raw unchecked JS.

**Status:** Approved · **Area:** Language · types · [review index](review.md)

## Goal fit

- **R1** typed, checked web code
- **R3** browser target
- **R0 / TC** compile cost controlled

## 1. Problem

There is no TypeScript-equivalent DOM/web type surface. Browser code and
`#js`/universal components are unchecked (AGENTS blocker) — agents cannot tell
correct DOM code from broken DOM code at compile time.

## 2. Decision (from review)

**Source:** a **first-party `dom` module shipped with the compiler/std**, generated
once from the official Web IDL and **versioned with the compiler**. Shipping avoids
the remote-import version drift that a GitHub-hosted package would introduce.

## 3. Proposed approach

1. **Submodules to control cost.** `dom` is split (e.g. `document`, `element`,
   `event`, `fetch`, …); only imported submodules are processed, so non-web builds
   never pay for them.
2. **Plain typed declarations.** Declarations are ordinary Chemical types marked
   `@extern`; they add no new front-end machinery and compile like any library.
3. **Backend mapping.** The JS backend (07) maps them to browser globals; on
   native/JVM they are simply _unavailable_ (using one is a clear compile error).
   Other backends are unaffected.
4. **Usage.**

   ```chemical
   import dom;
   var el = dom.document.get_element_by_id("app");
   el.set_attribute("class", "ready");
   ```

   Universal components reuse the same types for props/events, so their generated
   JS is typed too.

5. **Host-handle rule (proposed).** DOM objects are host-managed (browser GC).
   References to them are **non-owning handles**: freely copyable, not subject to
   lifetime checking, with aliasing relaxed for handles so ordinary DOM code passes
   the borrow checker (04). This keeps lifetime/use-after-free concerns off host
   objects.

## 4. Alternatives considered

- **Generate from Web IDL at build time**: always current, but adds a generator to
  every build and churn — rejected for a shipped, versioned module.
- **Generate from TypeScript's `lib.dom.d.ts`**: familiar/complete, but a licensing
  and format mismatch — rejected.
- **Remote GitHub package**: rejected — version drift; the compiler must not depend
  on the network for core types.

## 5. Phased milestones

1. Generate the `dom` module from Web IDL; split into submodules; ship with the
   build.
2. Core surface: `Window`, `Document`, `Element`, `Event`, `fetch`.
3. Wire to the JS backend (07) and to universal-component props/events.
4. Host-handle borrow rule (with 04).
5. A real browser program type-checks and runs; add tests.

## 6. Acceptance criteria

- A DOM program (`import dom;`) type-checks and runs in a browser.
- Non-web builds are unaffected in compile time and size (submodules not imported
  are not processed).
- Using `dom` on a backend that cannot host it fails with a clear diagnostic.
- Universal components share the types, so their JS is checked.

## 7. Risks & open questions

- Confirming the host-handle rule against the borrow checker without weakening
  owned code.
- Keeping the generated module's size manageable as Web IDL grows.
- How much of the API to cover in v1 (start with the core listed above).

## 8. Dependencies

Depends on: 07 (JS backend), 04 (borrow checker) · Unblocks: safe, typed universal
components.

## 9. Evidence & references

- AGENTS.md blocker "no TypeScript-equivalent DOM/web type declarations".
- `lang/docs/language-goals.md` §6 (runs-everywhere blockers).
- `lang/docs/proposals/DECISIONS.md` (second pass — source decision).
