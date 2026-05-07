/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EVALUATE_H_INCLUDED
#define EVALUATE_H_INCLUDED

#include <string>

#include "types.h"

namespace Stockfish {

class Position;

namespace Eval {

// The default net name MUST follow the format nn-[SHA256 first 12 digits].nnue
// for the build process (profile-build and fishtest) to work. Do not change the
// name of the macro or the location where this macro is defined, as it is used
// in the Makefile/Fishtest.
#define EvalFileDefaultNameBig "nn-c288c895ea92.nnue"
#define EvalFileDefaultNameSmall "nn-37f18f62d772.nnue"

namespace NNUE {
struct Networks;
struct AccumulatorCaches;
class AccumulatorStack;
}

std::string trace(Position& pos, const Eval::NNUE::Networks& networks);

// stockfish-ml-extensions: lets callers override the dynamic small/large
// network selection that `evaluate()` performs by default. `Auto` (the
// default) preserves vanilla SF18 behavior — pick small when the simple
// material eval is large enough, optionally re-evaluate with big when the
// small-net result is close to zero. `Small` / `Large` force the choice
// uniformly and skip the re-evaluation. Wired to the `NetSelection` UCI
// option in engine.cpp (`auto` / `small` / `large`).
enum class NetChoice { Auto, Small, Large };

int   simple_eval(const Position& pos);
bool  use_smallnet(const Position& pos);
Value evaluate(const NNUE::Networks&          networks,
               const Position&                pos,
               Eval::NNUE::AccumulatorStack&  accumulators,
               Eval::NNUE::AccumulatorCaches& caches,
               int                            optimism,
               NetChoice                      choice = NetChoice::Auto);

// stockfish-ml-extensions: same evaluation as `evaluate()`, but also exposes
// the raw NNUE per-head outputs that fed it. `v` is identical to what
// `evaluate()` would return (post-processed: head-blend + complexity damp +
// material/optimism mix + 50-move shuffling damp + TB-range clamp). `psqt`
// and `positional` are the raw `Value` outputs from `Networks::evaluate()` —
// post any auto-mode re-evaluation with the big net, so the pair always
// matches the network whose v was returned. All three are side-to-move POV.
//
// Use case: distillation labelling for hot-swap NNUE replacements, where the
// student must reproduce `(psqt, positional)` and Stockfish itself applies
// the post-processing on top. Plain policy / play distillation should keep
// using `evaluate()` — `v` is what makes Stockfish strong, and re-deriving
// it from `(psqt, positional)` requires reimplementing the post-processing
// in the consumer.
struct EvalComponents {
    Value v;
    Value psqt;
    Value positional;
};

EvalComponents evaluate_with_components(const NNUE::Networks&          networks,
                                        const Position&                pos,
                                        Eval::NNUE::AccumulatorStack&  accumulators,
                                        Eval::NNUE::AccumulatorCaches& caches,
                                        int                            optimism,
                                        NetChoice                      choice = NetChoice::Auto);
}  // namespace Eval

}  // namespace Stockfish

#endif  // #ifndef EVALUATE_H_INCLUDED
