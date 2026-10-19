# Chemical — Feature Proposals

This directory holds design proposals for the **large features** the language needs
to become *featureful* (not production-ready). Once every in-scope feature has an
approved proposal, the focus moves from breadth to **performance and stability**.

## Process (how a proposal becomes a document)

1. **Short version** — a one-screen summary (problem, direction, scope, open
   questions, goal fit) is added to the combined review file
   [`review.md`](review.md).
2. **Review** — the maintainer approves or rejects each proposal **in chat**. The
   short versions are reviewed one at a time.
3. **Approved** → the full proposal is written as
   `NNNN-<slug>.md` (also in this directory) and linked from `review.md`.
   **Rejected** → the short version is deleted, and any goal/requirement
   clarification it exposed is recorded in
   [`../language-goals.md`](../language-goals.md) and the relevant skill.
4. Approved proposals are **design documents only**. Implementation is a separate
   later phase.

## Layout

```
lang/docs/proposals/
  README.md              # this file
  review.md              # the combined short-version review file
  template.md            # the shared Markdown template full proposals are stamped from
  DECISIONS.md           # review decisions log
  NNNN-<slug>.md         # full proposals, once approved
```

## Conventions

- Proposals are numbered in dependency order: `0001`, `0002`, ….
- Proposals are plain Markdown — no shared stylesheet, script or build step; they
  render as-is on GitHub and in any Markdown viewer/editor.
- Full proposal depth: **design + phased milestones + acceptance criteria + risks +
  goal fit** (see `template.md`).
- Every proposal carries a **Goal fit** section mapping to the four requirements
  (`../language-goals.md` §2) and the toolchain constraint (small, fast, everywhere).
