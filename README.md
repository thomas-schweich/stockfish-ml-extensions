# `stockfish-ml-extensions`

A fork of [official-stockfish/Stockfish](https://github.com/official-stockfish/Stockfish)
pinned to the **Stockfish 18** release (commit
[`cb3d4ee9`](https://github.com/official-stockfish/Stockfish/commit/cb3d4ee9b47d0c5aae855b12379378ea1439675c),
tag `sf_18`) hosting small, additive UCI extensions useful for machine-learning
workflows — supervised data generation, distillation labelling, policy
extraction, and similar. Built primarily for the
[PAWN](https://github.com/thomas-schweich/pawn) self-play data pipeline, but
the extensions are general-purpose and standalone.

Every extension is **purely additive**: the UCI dispatcher gains a new command,
nothing existing changes. Every standard Stockfish command (`go nodes N`,
`eval`, `bench`, `position`, the search path, the NNUE weights bundled with
SF18) is **bit-identical** to vanilla Stockfish 18. Drop-in replacement for
any UCI consumer.

## Extensions

### `NetSelection` — force one NNUE network for all evaluation

Vanilla Stockfish 18 dynamically picks between the small and big NNUE
networks based on the position's material balance (`use_smallnet`), and
optionally re-evaluates small-net results with the big net when the small
result is close to zero. Useful for playing strength, but introduces
position-dependent eval-source heterogeneity that's awkward for ML use
cases — especially distillation labelling, where you want a uniform
teacher network across the dataset.

UCI option `NetSelection` (combo: `auto | small | large`, default `auto`):

```
setoption name NetSelection value large
```

- `auto`  — preserves vanilla SF18 behavior bit-for-bit.
- `small` — always use the small net, never re-evaluate.
- `large` — always use the big net, no re-eval needed (it's already big).

Effective for both the standard search path (`go nodes N` and friends —
read once at `start_searching`, cached on the search worker for the search
lifetime) and the `evallegal` command below (read fresh on each call).
Default `auto` means existing callers see no behavior change.

### `evallegal` — per-legal-move NNUE eval, single line

For every legal move at the current position, plays the move, runs
`Eval::evaluate` on the resulting position, and undoes the move — emitting a
single line summarizing all of them:

```
info string evallegal <status> [<uci> <cp> <v>]...
```

- `<status>` is one of `none`, `check`, `mate`, `stalemate`.
- For `none` / `check`, the rest of the line is space-separated triplets
  `<uci> <cp> <v>`, one per legal move.
- `<cp>` is the normalized centipawn value (`UCIEngine::to_cp(v, pos)`),
  matching `info ... score cp N` lines from a normal search.
- `<v>` is the raw internal `Value` the NNUE produced before normalization —
  the right target for distillation losses, and the network's actual policy
  logit before the per-position win-rate-model normalization shrinks
  magnitudes by `a / 100` ≈ 2–3.5×.
- Both scores are mover-POV (negated from the post-move side-to-move POV
  that `Eval::evaluate` returns).

The command bypasses the entire search loop — no thread spawn, no MultiPV
`info` chatter, no TT/history bookkeeping, no `bestmove` round-trip. Just
move generation + per-child NNUE forward + undo + one synchronized output.

Example (startpos, where every move evaluates symmetrically):

```
info string evallegal none a2a3 0 0 b2b3 0 0 ... g1h3 0 0
```

Example (in-check position with one legal escape):

```
info string evallegal check h8g8 -510 -1981
```

Example (terminal positions):

```
info string evallegal mate
info string evallegal stalemate
```

## Compatibility & licensing

All extensions are gated behind new UCI commands or options that vanilla
Stockfish doesn't recognize — vanilla GUIs and tools see this fork as
identical to Stockfish 18, and any consumer that *does* know about the new
commands gets the extra functionality.

GPLv3, inherited from upstream Stockfish (see `Copying.txt`).

## Build

Same as upstream:

```bash
cd src && make -j build ARCH=x86-64-avx2
```

Verify the extensions are present:

```
$ printf 'evallegal\nquit\n' | ./stockfish 2>/dev/null | head
Stockfish 18 by the Stockfish developers (see AUTHORS file)
info string evallegal none a2a3 0 0 b2b3 0 0 c2c3 0 0 ... g1h3 0 0
```

Vanilla SF18 would respond `Unknown command: 'evallegal'` instead.

## Updating from upstream

This fork tracks `sf_18` only. To rebase onto a newer Stockfish release,
cherry-pick the extension commits onto the new base; each extension is a
single self-contained commit, and the touched files (`engine.{h,cpp}`,
`uci.cpp`) don't churn heavily upstream.

---

<div align="center">

  [![Stockfish][stockfish128-logo]][website-link]

  <h3>Stockfish</h3>

  A free and strong UCI chess engine.
  <br>
  <strong>[Explore Stockfish docs »][wiki-link]</strong>
  <br>
  <br>
  [Report bug][issue-link]
  ·
  [Open a discussion][discussions-link]
  ·
  [Discord][discord-link]
  ·
  [Blog][website-blog-link]

  [![Build][build-badge]][build-link]
  [![License][license-badge]][license-link]
  <br>
  [![Release][release-badge]][release-link]
  [![Commits][commits-badge]][commits-link]
  <br>
  [![Website][website-badge]][website-link]
  [![Fishtest][fishtest-badge]][fishtest-link]
  [![Discord][discord-badge]][discord-link]

</div>

## Overview

[Stockfish][website-link] is a **free and strong UCI chess engine** derived from
Glaurung 2.1 that analyzes chess positions and computes the optimal moves.

Stockfish **does not include a graphical user interface** (GUI) that is required
to display a chessboard and to make it easy to input moves. These GUIs are
developed independently from Stockfish and are available online. **Read the
documentation for your GUI** of choice for information about how to use
Stockfish with it.

See also the Stockfish [documentation][wiki-usage-link] for further usage help.

## Files

This distribution of Stockfish consists of the following files:

  * [README.md][readme-link], the file you are currently reading.

  * [Copying.txt][license-link], a text file containing the GNU General Public
    License version 3.

  * [AUTHORS][authors-link], a text file with the list of authors for the project.

  * [src][src-link], a subdirectory containing the full source code, including a
    Makefile that can be used to compile Stockfish on Unix-like systems.

  * a file with the .nnue extension, storing the neural network for the NNUE
    evaluation. Binary distributions will have this file embedded.

## Contributing

__See [Contributing Guide](CONTRIBUTING.md).__

### Donating hardware

Improving Stockfish requires a massive amount of testing. You can donate your
hardware resources by installing the [Fishtest Worker][worker-link] and viewing
the current tests on [Fishtest][fishtest-link].

### Improving the code

In the [chessprogramming wiki][programming-link], many techniques used in
Stockfish are explained with a lot of background information.
The [section on Stockfish][programmingsf-link] describes many features
and techniques used by Stockfish. However, it is generic rather than
focused on Stockfish's precise implementation.

The engine testing is done on [Fishtest][fishtest-link].
If you want to help improve Stockfish, please read this [guideline][guideline-link]
first, where the basics of Stockfish development are explained.

Discussions about Stockfish take place these days mainly in the Stockfish
[Discord server][discord-link]. This is also the best place to ask questions
about the codebase and how to improve it.

## Compiling Stockfish

Stockfish has support for 32 or 64-bit CPUs, certain hardware instructions,
big-endian machines such as Power PC, and other platforms.

On Unix-like systems, it should be easy to compile Stockfish directly from the
source code with the included Makefile in the folder `src`. In general, it is
recommended to run `make help` to see a list of make targets with corresponding
descriptions. An example suitable for most Intel and AMD chips:

```
cd src
make -j profile-build
```

Detailed compilation instructions for all platforms can be found in our
[documentation][wiki-compile-link]. Our wiki also has information about
the [UCI commands][wiki-uci-link] supported by Stockfish.

## Terms of use

Stockfish is free and distributed under the
[**GNU General Public License version 3**][license-link] (GPL v3). Essentially,
this means you are free to do almost exactly what you want with the program,
including distributing it among your friends, making it available for download
from your website, selling it (either by itself or as part of some bigger
software package), or using it as the starting point for a software project of
your own.

The only real limitation is that whenever you distribute Stockfish in some way,
you MUST always include the license and the full source code (or a pointer to
where the source code can be found) to generate the exact binary you are
distributing. If you make any changes to the source code, these changes must
also be made available under GPL v3.

## Acknowledgements

Stockfish uses neural networks trained on [data provided by the Leela Chess Zero
project][lc0-data-link], which is made available under the [Open Database License][odbl-link] (ODbL).


[authors-link]:       https://github.com/official-stockfish/Stockfish/blob/master/AUTHORS
[build-link]:         https://github.com/official-stockfish/Stockfish/actions/workflows/stockfish.yml
[commits-link]:       https://github.com/official-stockfish/Stockfish/commits/master
[discord-link]:       https://discord.gg/GWDRS3kU6R
[issue-link]:         https://github.com/official-stockfish/Stockfish/issues/new?assignees=&labels=&template=BUG-REPORT.yml
[discussions-link]:   https://github.com/official-stockfish/Stockfish/discussions/new
[fishtest-link]:      https://tests.stockfishchess.org/tests
[guideline-link]:     https://github.com/official-stockfish/fishtest/wiki/Creating-my-first-test
[license-link]:       https://github.com/official-stockfish/Stockfish/blob/master/Copying.txt
[programming-link]:   https://www.chessprogramming.org/Main_Page
[programmingsf-link]: https://www.chessprogramming.org/Stockfish
[readme-link]:        https://github.com/official-stockfish/Stockfish/blob/master/README.md
[release-link]:       https://github.com/official-stockfish/Stockfish/releases/latest
[src-link]:           https://github.com/official-stockfish/Stockfish/tree/master/src
[stockfish128-logo]:  https://stockfishchess.org/images/logo/icon_128x128.png
[uci-link]:           https://backscattering.de/chess/uci/
[website-link]:       https://stockfishchess.org
[website-blog-link]:  https://stockfishchess.org/blog/
[wiki-link]:          https://github.com/official-stockfish/Stockfish/wiki
[wiki-compile-link]:  https://github.com/official-stockfish/Stockfish/wiki/Compiling-from-source
[wiki-uci-link]:      https://github.com/official-stockfish/Stockfish/wiki/UCI-&-Commands
[wiki-usage-link]:    https://github.com/official-stockfish/Stockfish/wiki/Download-and-usage
[worker-link]:        https://github.com/official-stockfish/fishtest/wiki/Running-the-worker
[lc0-data-link]:      https://storage.lczero.org/files/training_data
[odbl-link]:          https://opendatacommons.org/licenses/odbl/odbl-10.txt

[build-badge]:        https://img.shields.io/github/actions/workflow/status/official-stockfish/Stockfish/stockfish.yml?branch=master&style=for-the-badge&label=stockfish&logo=github
[commits-badge]:      https://img.shields.io/github/commits-since/official-stockfish/Stockfish/latest?style=for-the-badge
[discord-badge]:      https://img.shields.io/discord/435943710472011776?style=for-the-badge&label=discord&logo=Discord
[fishtest-badge]:     https://img.shields.io/website?style=for-the-badge&down_color=red&down_message=Offline&label=Fishtest&up_color=success&up_message=Online&url=https%3A%2F%2Ftests.stockfishchess.org%2Ftests%2Ffinished
[license-badge]:      https://img.shields.io/github/license/official-stockfish/Stockfish?style=for-the-badge&label=license&color=success
[release-badge]:      https://img.shields.io/github/v/release/official-stockfish/Stockfish?style=for-the-badge&label=official%20release
[website-badge]:      https://img.shields.io/website?style=for-the-badge&down_color=red&down_message=Offline&label=website&up_color=success&up_message=Online&url=https%3A%2F%2Fstockfishchess.org
