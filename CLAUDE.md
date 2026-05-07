# CLAUDE.md

This file briefs Claude (and other LLM-assisted contributors) on what this
repo is, what it isn't, and how to extend it without breaking the contract
its downstream consumers rely on.

## What this repo is

A **fork of [official-stockfish/Stockfish](https://github.com/official-stockfish/Stockfish)**
pinned to the SF18 release (`cb3d4ee9`, tag `sf_18`) that hosts a small set
of additive UCI extensions useful for machine-learning workflows —
supervised data generation, distillation labelling, policy extraction, and
similar.

The fork's primary downstream consumer is the
[PAWN](https://github.com/thomas-schweich/pawn) self-play data pipeline,
which pins this repo by SHA in its build script and Docker image. Other
projects can use it as a drop-in SF18 replacement.

### What it is NOT

- **Not a strength fork.** No search changes. No NNUE retraining. No new
  tuning. If you're tempted to touch `search.cpp` for play-strength
  reasons, send the patch upstream instead.
- **Not a long-lived divergent branch.** Each extension lives as a single
  self-contained commit on top of `sf_18` so it can be cherry-picked onto
  a future SF release with minimal effort.
- **Not a place for engine bugs.** Anything not specific to the fork's
  extensions belongs in the upstream Stockfish issue tracker — the issue
  templates here enforce that.

## Repo layout

Everything under `src/`, `tests/`, `scripts/`, `AUTHORS`, `Copying.txt`,
`Top CPU Contributors.txt`, `CONTRIBUTING.md`, `.clang-format`,
`.git-blame-ignore-revs` is **upstream Stockfish 18** verbatim (modulo the
extension commits). Do not touch upstream code for cosmetic reasons —
keep the rebase surface minimal.

The fork's own contribution is:

- `src/engine.{h,cpp}` — additive: `evallegal_legal_moves()` method,
  `NetSelection` option registration. Existing public API unchanged.
- `src/uci.cpp` — additive: one new dispatcher branch for `evallegal`.
- `src/evaluate.cpp`, `src/search.cpp` — additive reads of the
  `NetSelection` option (see "Driving principles" — the option is read
  through pre-existing code paths, not via new ones).
- `README.md` — fork-overview section above the upstream README content.
- `.github/ISSUE_TEMPLATE/` — fork-scoped templates redirecting non-fork
  issues upstream.
- `CLAUDE.md` — this file.
- `CITATION.cff` — **upstream Stockfish citation, intentionally
  unmodified** (see "On `CITATION.cff`" below).

There are **no GitHub Actions** in this repo. Upstream's CI matrix
(compilation matrices, sanitizers, matetrack, IWYU, codeql, binary upload)
was removed in `148f50a3`. We don't have the test infrastructure to
maintain it for this fork's narrow scope, and engine-quality regressions
should be caught upstream long before they land here.

## The two extensions

### `evallegal` (UCI command, sf18-v0.1.0+)

Per-legal-move NNUE eval emitted as one synchronized line:

```
info string evallegal <status> [<uci> <cp> <v>]...
```

- Status: `none | check | mate | stalemate`. For `none` / `check`, triplets
  follow; for `mate` / `stalemate`, the line ends after the status.
- `<cp>` = `UCIEngine::to_cp(v, pos)` (normalized centipawn, matches
  `info ... score cp N` from a search).
- `<v>` = raw internal NNUE `Value` before normalization — the right
  target for distillation losses.
- Both are mover-POV.

Implementation: `src/engine.cpp` (`evallegal_legal_moves`, ~line 360),
dispatcher branch in `src/uci.cpp:151`. Bypasses the entire search loop —
no thread spawn, no MultiPV chatter, no TT/history bookkeeping, no
`bestmove`. Just `MoveList<LEGAL>` + per-child NNUE forward + undo + one
`sync_cout`.

Heap-allocates `AccumulatorStack` / `AccumulatorCaches` via `unique_ptr`
(matching the `trace_eval` pattern) — the stack version trips
`-Wstack-usage=128000` at ~2.8 MB.

### `NetSelection` (UCI option, sf18-v0.2.0+)

Combo option `auto | small | large`, default `auto`. Forces uniform use
of one NNUE network across all evaluation, instead of vanilla SF18's
position-dependent dynamic small/big pick (`use_smallnet` based on
`simple_eval` magnitude).

Read in two places:

- `src/search.cpp:187` — cached on the search worker once at
  `start_searching` for the lifetime of a search (no per-node option
  lookup).
- `src/engine.cpp:360` — read fresh per `evallegal` call (one
  `options[...]` lookup amortizes over all legal moves).

Default `auto` preserves vanilla SF18 behavior bit-for-bit.

## Adding a new extension

Before writing code, check whether the change actually needs to live in
this fork. If it doesn't depend on Stockfish internals (Position, NNUE,
Eval), it should be a downstream tool that pipes UCI to vanilla SF —
not a fork patch.

If it does belong here, here's the checklist. The two existing extensions
are good templates for command-style (`evallegal` → `b9eec477`) vs.
option-style (`NetSelection` → `a225c1b8`) additions.

### Code

For a new **UCI command** (like `evallegal`):

1. New public method on `Engine` (`src/engine.{h,cpp}`).
2. New dispatcher branch in `src/uci.cpp` — match `else if (token == "...")`
   pattern. Comment with `// pawn datagen extension` (or whichever
   downstream is asking for it) so future rebasers know why it exists.
3. If your command emits to stdout, use `sync_cout … sync_endl` —
   never raw `std::cout`. Other UCI commands and `info` lines must not
   be interleaved.

For a new **UCI option** (like `NetSelection`):

1. `options.add("OptionName", Option("type-spec", "default"))` in
   `Engine::Engine()` in `src/engine.cpp`.
2. **Read it through pre-existing code paths.** `NetSelection` is read
   inside `Eval::evaluate` (or its caller) where `use_smallnet` was
   already being computed — we override an already-computed value rather
   than introducing a new control-flow split. Adding new `if (option)`
   branches at the call sites makes the rebase surface ugly.
3. If the option is read on a hot path (per-node, per-eval), cache it
   once at search start instead of looking it up every call. See
   `src/search.cpp:187` for the pattern.

### Defaults

**Defaults must reproduce vanilla SF18 behavior bit-for-bit.** A user who
runs the fork without setting any new options should get identical
output to vanilla SF18 on every input. This is non-negotiable — any
consumer relying on the fork as a drop-in SF18 replacement depends on it.

If you can't make the default a no-op (e.g., the extension only makes
sense when active), reconsider whether it belongs in the fork at all.

### Tests

Upstream's test infrastructure was removed with the workflows. For
fork-specific tests, write a small shell script in `tests/` that pipes
UCI to a built binary and greps for the expected output shape. Do not
re-introduce GitHub Actions — the downstream PAWN pipeline runs a smoke
test on every Docker build (`Dockerfile.datagen` `sf-fork-build-1`
stage) which is a sufficient regression gate.

### Docs to update on each new extension

Inside this repo:

1. `README.md` "Extensions" section — add a new subsection in the same
   style as the existing two. Include the UCI shape, what it produces,
   and the rationale.
2. `CLAUDE.md` "The two extensions" section becomes "The N extensions"
   — document the new entry point, file:line pointers, and any
   non-obvious implementation notes.
3. **Tag a new release** (`sf18-v0.X.Y` semver: bump minor for
   additive features, patch for bugfixes). Annotated tags only —
   downstream pins by SHA but tags exist for humans. Never force-move
   an existing tag; old PAWN builds depend on `sf18-v0.1.0` and
   `sf18-v0.2.0` resolving to the same SHAs forever.

In the **PAWN repo** (`thomas-schweich/pawn`), if PAWN is going to use
the new capability:

1. `stockfish-datagen/scripts/build_patched_stockfish.sh` — bump
   `SF_COMMIT` to the new SHA.
2. `stockfish-datagen/Dockerfile.datagen` — bump the `sf-fork-build-1`
   stage's clone SHA.
3. `stockfish-datagen/patches/README.md` — describe the new capability
   and which preflight gate (if any) checks for it.
4. `stockfish-datagen/src/main.rs` — if the capability is required by
   any tier or subcommand, add a preflight check. The pattern is in
   `preflight_check_patched_binary` (gates the runner) and
   `preflight_check_tournament_binary` (gates the tournament
   subcommand). Different capabilities → different gates so users on
   older fork builds get a clear error rather than silent no-ops.
5. `stockfish-datagen/src/stockfish.rs` — if the new capability needs to
   be detected during the UCI handshake (e.g., a new option name),
   add a probe field on `StockfishProcess` set during `spawn`.

The PAWN-side updates are **only** needed if PAWN actually uses the
capability. A new fork feature that no downstream uses yet ships in a
new tag and waits to be picked up.

## Driving principles

1. **Additive only.** Every patch must be a strict superset of vanilla
   SF18 behavior. New commands the dispatcher didn't have, new options
   that default to no-op. Existing UCI traffic is bit-identical to
   vanilla. This is what makes the fork a drop-in replacement and what
   makes rebases trivial.
2. **One self-contained commit per extension.** Don't spread an
   extension across three commits "to make review easier" — when SF19
   ships and we rebase, we want to cherry-pick three commits cleanly,
   not untangle a fix-up chain. Squash before merging to `main`.
3. **No GPLv3 boundary leaks.** The fork is GPLv3, inherited from
   Stockfish. Downstream consumers (e.g., PAWN, which is Apache 2.0)
   interact with it only over the UCI process boundary. Don't link
   anything from this repo into an Apache-licensed library, and don't
   pull Apache-licensed code into this repo. The whole point of
   splitting this out from PAWN was to keep that boundary clean.
4. **Don't fight upstream.** If a planned extension would conflict with
   upstream's direction (e.g., touching `Eval::evaluate`'s control
   flow in a way SF19 will likely change), accept the conflict cost
   or rethink the design. The bus factor of this fork is one; we
   minimize divergence to keep it survivable.
5. **Pin by SHA, not tag, in build scripts.** Tags are human-readable
   pointers. Build scripts must use commit SHAs so a moved tag
   doesn't silently change a deployed binary. The fork itself never
   force-moves tags, but downstreams shouldn't trust that.
6. **Hot-path lookups are cached, not repeated.** UCI options are
   read from a synchronized map; reading them per-node tanks NPS.
   Cache at search start, refresh once. See `NetSelection` in
   `src/search.cpp:187` for the pattern.
7. **If you find yourself reaching for `std::cout`, stop.** UCI output
   must use `sync_cout … sync_endl` to interleave correctly with
   `info` lines and `bestmove`. The existing extensions follow this
   contract; new ones must too.

## On `CITATION.cff`

The current `CITATION.cff` cites the upstream Stockfish project and is
**intentionally unmodified**. Recommendation: leave it that way.

Reasoning:

- This fork is a small engineering layer on top of an already-citable
  research artifact (Stockfish + NNUE). It is not itself a research
  contribution that someone would cite in a paper — there's no novel
  algorithm, no benchmark result, no theoretical claim.
- If a paper uses data generated via this fork, the right citation is
  Stockfish 18 (the actual eval source), with this fork's GitHub URL
  mentioned in a footnote or methods section as the implementation
  detail. That's already what `CITATION.cff` produces.
- Replacing the citation with a fork-specific one would fragment the
  citation graph and make it harder for readers to find the actual
  upstream work. Adding a `references:` block pointing back at
  Stockfish would be redundant with what's already there.

The one case where this changes is if we ever publish a paper or
preprint specifically about the extensions or a downstream method that
critically depends on them. At that point, switch to a
`preferred-citation:` block pointing at the paper, and keep the
Stockfish citation in `references:`. Until then, `CITATION.cff` stays
as-is.

If a contributor proposes editing `CITATION.cff`, push back unless one
of those conditions is met. It's the kind of file that gets touched
casually and ends up incorrect.
