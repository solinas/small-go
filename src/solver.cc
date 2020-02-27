// Copyright 2019 Chris Solinas
#include "solver.h"
#include <iostream>
#include <algorithm>

Solver::Solver() : nodes(0), verbose(true) { init_theorems_3x3(); }

int Solver::solve(Go *game, Color c) {
  int max_score = game->size() * game->size();
  return solve(game, c, max_score);  
}

int Solver::solve(Go *game, Color c, int max_score) {
  nodes = 0;
  start = Clock::now();
  int max_depth = 0;
  
  Result r;

  while (r.is_undefined()) {
    r = alpha_beta(game, c, -1.0 * max_score, 1.0 * max_score, 0, ++max_depth);
    r.pv.push_front(r.best_move);
    if (verbose) {
      display_results(r, max_depth);
    }
  }
  return r.best_move;
}

Result Solver::alpha_beta(Go *game, Color c, float alpha, float beta, int d,
    int max_depth) {

  Result best;
  if (d > max_depth) return best;

  if (game->game_over() || (MAX_NODES > 0 && nodes > MAX_NODES)) {
    best.value = game->score(c);
    best.terminal = true;
    return best;
  }

  if (game->size() == 3) {
    for (Theorem *t : theorems_3x3) {
      if (t->applies(game->get_board(), Go::opponent(c))) {
        best.value = -1 * t->get_value();
        best.terminal = true;
        best.benson = true;
        return best;
      }
    }
  }

  // now check transposition table to see if we found an isomorphism
  long current_path = game->get_current_path();
  if (TT.find(current_path) != TT.end() && TT[current_path].to_move == c) {
    if (TT[current_path].max_depth >= max_depth || TT[current_path].res.terminal) {
      return TT[current_path].res;
    }
  }
  
  nodes += 1;

  // generate and sort moves
  std::vector<int> moves;
  game->get_moves(&moves);
  if (game->size() == 3) {
    std::sort(moves.begin(), moves.end(),
      move_ordering_3x3(game->get_board(), c));
  } else if (game->size() == 2) {
    std::sort(moves.begin(), moves.end(), move_ordering_2x2());
  }

  bool undefined = false;
  for (auto move : moves) {
    bool legal = game->make_move(move, c);
    if (!legal) continue;
    Result r = alpha_beta(game, Go::opponent(c), -1 * beta, -1 * alpha, d + 1,
        max_depth);

    r.pv.push_front(r.best_move);
    r.best_move = move;
    // negamax variant
    r.value *= -1;
    game->undo_move();

    if (r.is_undefined()) {
      undefined = true;
      continue;
    }

    if (r > best) best = r;

    if (r.value > alpha) {
      alpha = r.value;
    }
    // pruning
    if (alpha >= beta || best.benson) break;
  }

  if (!best.benson && undefined) best.reset();

  if (!undefined && !game->last_move_was_pass()) {
    add_iso_pos_to_TT(game->get_isomorphic_paths(),
        game->get_isomorphic_moves(best.best_move), best.value, c, 
        best.terminal, max_depth);
  }

  best.benson = false;

  return best;
}

void Solver::display_intermediate() {
  auto dur = std::chrono::duration_cast<float_seconds>(Clock::now() - start);
  std::cout << "nodes: " << nodes;
  std::cout << " nodes/sec: " << nodes / dur.count() << std::endl;
}

void Solver::display_results(Result r, int max_depth) {
  auto dur = std::chrono::duration_cast<float_seconds>(Clock::now() - start);
  std::cout << "d: " << max_depth;
  if (!r.is_undefined())
    std::cout << " value: " << r.value << " move: " << r.best_move;
  else
    std::cout << " undefined";
  std::cout << " nodes: " << nodes;
  std::cout << " nodes/sec: " << nodes / dur.count() << std::endl;

  if (!r.is_undefined()) {
    std::cout << "pv:";
    for (int m : r.pv) {
      if (m >= -1) std::cout << " " << m << " ";
    }
    std::cout << std::endl;
  }
}

void Solver::init_theorems_3x3() {
  theorems_3x3.push_back(new Middle3x3());
  theorems_3x3.push_back(new Corner3x3());
  theorems_3x3.push_back(new SideOnly3x3());
}

void Solver::clean_theorems_3x3() {
  for (auto t : theorems_3x3) {
    delete t;
  }
}

void Solver::add_iso_pos_to_TT(const std::array<long, NUM_ISO>& batch, 
    std::array<int, NUM_ISO> iso_moves, float value, Color to_move,
    bool terminal, int max_depth) {
  if (batch[0] == 0) return;
  for (int i = 0; i < NUM_ISO; i++) {
    if (TT.find(batch[i]) == TT.end() || !TT[batch[i]].res.terminal) {
      Result r;
      r.value = value;
      r.best_move = iso_moves[i];
      r.terminal = terminal;
      TT_entry entry(r, to_move, max_depth);
      TT[batch[i]] = entry;
    }
  }
}
