# Chemical — Feature Proposals

This directory holds design proposals for the **large features** the language needs
to become *featureful* (not production-ready). Once every in-scope feature has an
approved proposal, the focus moves from breadth to **performance and stability**.

## Process (how a proposal becomes a document)

1. **Short version** — a one-screen summary (problem, direction, scope, open
   questions, goal fit) is added to the combined review file
   [`review.html`](review.html). Review it in a browser.
2. **Review** — the maintainer approves or rejects each proposal **in chat**. The
   short versions are reviewed one at a time.
3. **Approved** → the full proposal is written as
   `NNNN-<slug>.html` (also in this directory) and linked from `review.html`.
   **Rejected** → the short version is deleted, and any goal/requirement
   clarification it exposed is recorded in
   [`../language-goals.md`](../language-goals.md) and the relevant skill.
4. Approved proposals are **design documents only**. Implementation is a separate
   later phase.

## Layout

```
lang/docs/proposals/
  README.md              # this file
  review.html            # the combined short-version review file (open in a browser)
  template.html          # the shared HTML template full proposals are stamped from
  assets/proposal.css    # shared stylesheet
  NNNN-<slug>.html       # full proposals, once approved
```

## Conventions

- Proposals are numbered in dependency order: `0001`, `0002`, ….
- Full proposal depth: **design + phased milestones + acceptance criteria + risks +
  goal fit** (see `template.html`).
- Every proposal carries a **Goal fit** section mapping to the four requirements
  (`../language-goals.md` §2) and the toolchain constraint (small, fast, everywhere).
