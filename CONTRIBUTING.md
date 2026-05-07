# Contributing

This is a narrow-scope fork of Stockfish 18 that hosts a small set of
**additive, ML-workflow-oriented UCI extensions** (`evallegal`,
`NetSelection`). For the project's purpose, design constraints, and a
checklist for adding extensions, read [CLAUDE.md](./CLAUDE.md) first.

## Most contributions belong upstream

If your contribution is about Stockfish itself — search behavior, NNUE
quality, eval improvements, build/portability fixes, UCI protocol
compliance, performance tuning, code style, vanilla bug reports — it
belongs in [official-stockfish/Stockfish][upstream], not here. We do
not patch search or evaluation code in this fork, and we do not have
the test infrastructure (Fishtest, the CI matrix, sanitizer runs,
matetrack) needed to validate changes that affect engine strength.

Filing such an issue or PR here will get it closed and redirected. It's
not a rejection of the work — it's that the right reviewers, the right
tests, and the right merge path all live upstream.

## When a contribution does belong here

There is exactly one class of contribution this fork accepts:

> **An additive, generally-useful, ML-oriented modification to Stockfish
> that would fit well into this fork (small surface area, no impact on
> default-config behavior, useful for data generation / distillation /
> probing / similar workflows) and poorly upstream (no play-strength
> benefit, so Fishtest can't validate it and upstream policy declines
> non-strength features).**

Examples of contributions that fit:

- A new UCI command that emits internal state useful for ML labelling
  (e.g., per-position policy logits, NNUE accumulator dumps, search-tree
  shape statistics).
- A new UCI option that makes existing behavior more deterministic or
  uniform for dataset generation (e.g., disabling some position-dependent
  heuristic that adds heterogeneity to eval sources).
- A non-functional improvement to an existing extension's output format
  or performance, with no behavior change at default settings.

Examples of contributions that do **not** fit, even if they're additive:

- Anything that changes default-config behavior. Hard rule.
- New search algorithms, eval ideas, or NNUE architectures — those want
  Fishtest, which lives upstream.
- Tooling that doesn't depend on Stockfish internals (Position, NNUE,
  Eval). Those should be standalone projects that pipe UCI to vanilla
  SF, not patches in this fork.
- Anything specific to one downstream consumer. The bar is "generally
  useful for ML workflows," not "useful for one project."

If you're unsure whether your idea fits, **open an issue first** using
the ML-extension issue template and describe the use case. A short
discussion before you write code saves both sides time.

## How to propose a change

If your contribution clears the bar above:

1. **Open an issue first** (or comment on an existing one) describing
   the proposed extension: the use case, the UCI shape, and a sketch
   of the implementation. Wait for confirmation that the scope fits
   before writing code.
2. **Fork this repo and open a PR.** The PR target is `main`.
3. **Read [CLAUDE.md](./CLAUDE.md)'s "Adding a new extension" section
   and "Driving principles" before writing the patch.** The
   non-negotiable items:
   - Additive only — defaults must reproduce vanilla SF18 behavior
     bit-for-bit.
   - One self-contained commit per extension. Squash before merge.
   - No churn in upstream-vanilla files. Don't reformat, rename, or
     refactor upstream code "while you're there" — every diff against
     `sf_18` is rebase debt when the next SF release lands.
   - UCI output uses `sync_cout … sync_endl`, never raw `std::cout`.
   - Hot-path option lookups are cached at search start, not repeated
     per node.
4. **Update the docs in the same PR.** Specifically:
   - `README.md` — add a subsection under "Extensions".
   - `CLAUDE.md` — add the new extension to the documented list with
     file:line pointers and any non-obvious implementation notes.
5. **Tests.** This repo has no GitHub Actions and no Fishtest hookup.
   Add a small shell script under `tests/` that pipes UCI to a built
   binary and greps for the expected output shape. The downstream
   PAWN smoke test (in its `Dockerfile.datagen`) will exercise the
   binary on every build, so a basic shape check here is sufficient.

A maintainer will tag a new annotated release (`sf18-vX.Y.Z`) once the
PR merges. Tags are never force-moved — downstream consumers pin by
SHA but tags exist for humans, and old pins must keep resolving forever.

## Code style and license

Code style follows upstream Stockfish — `.clang-format` is unchanged
from upstream. Run `make format` (clang-format 20) before submitting.

By contributing, you agree your contribution is licensed under
**GPL v3.0**, inherited from upstream Stockfish (see `Copying.txt`).
This is non-negotiable — the GPLv3 boundary is what makes this fork
legally clean to depend on from a separately-licensed project (the
PAWN downstream is Apache 2.0 and interacts with this fork only over
the UCI process boundary).

The `AUTHORS` file is upstream Stockfish's contributor list; do not
add yourself there. Fork contributions are attributed via Git history
on this repo.

[upstream]: https://github.com/official-stockfish/Stockfish
