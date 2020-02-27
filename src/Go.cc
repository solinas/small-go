// Copyright 2019 Chris Solinas
#include "Go.h"

#include <iostream>
#include <cassert>

Go::Go(int _n) : to_move(BLACK), n(_n) {
  Board::init_zobrist();
  init_path_hash_table(n);
  // set up 8 boards for handling isomorphisms
  for (int i = 0; i < NUM_ISO; i++) {
    std::stack<Board> b;
    b.push(Board(_n));
    boards.push_back(b);
    std::stack<long> p;
    p.push(0);
    paths.push_back(p);
  }
  
  passes.push(0);
}

Go::~Go() {
 for (int i = 0; i < n; i++) {
   delete[] path_hash[i];
 }
 delete[] path_hash;
}

void Go::init_path_hash_table(int n) {
  path_hash = new long*[n*n];
  for (int i = 0; i < n*n; i++) {
    path_hash[i] = new long[MAX_DEPTH];
    for (int j = 0; j < MAX_DEPTH; j++) {
      path_hash[i][j] = std::rand();
    }
  }
}

int Go::size() { return n; }

bool Go::game_over() {
  return passes.top() > 1;
}

bool Go::last_move_was_pass() { return passes.top() > 0; }

bool Go::make_move(int point_ind, Color color) {

  // first copy
  for (int i = 0; i < NUM_ISO; i++) {
    boards[i].push(boards[i].top());
    paths[i].push(paths[i].top());
  }

  // check for a pass
  if (point_ind == PASS_IND) {
    passes.push(passes.top() + 1);
    if (to_move == color) {
      switch_to_move();
    }
    return true;
  }

  bool res = boards[0].top().move(point_ind, color);
  if (!res) {
    boards[0].pop();
    paths[0].pop();
  } else {
    // move succeeded, check superko
    if (superko_hist.find(boards[0].top().h) != superko_hist.end()) {
      res = false;
      boards[0].pop();
      paths[0].pop();
    } else {
      superko_hist.insert(boards[0].top().h);
    }
  }

  if (res) {
    // all checks done, reset pass counter, make moves on iso boards
    passes.push(0);
    paths[0].top() ^= path_hash[point_ind][boards[0].size()];
     
    if (n == 3) {
      // TODO flips/rotations hard-coded for 3x3 right now
      for (int i =1; i < NUM_ISO; i++) {
        //3 rotations, flip, 4 rotations
        if (i % 4 == 0) {
          point_ind = Board::horiz_flip_index_3x3[point_ind];
        }
        point_ind = Board::rotation90_index_3x3[point_ind];

        // make the isomorphic move
        res = boards[i].top().move(point_ind, color);
        paths[i].top() ^= path_hash[point_ind][boards[i].size()];
        assert(res);
      }
    }

    // other player's turn next
    if (to_move == color) {
      switch_to_move();
    }
  } else {
    // checks failed, remove copied boards
    for (int i =1; i < NUM_ISO; i++) {
      boards[i].pop();
      paths[i].pop();
    }
  }
    
  return res;
}

bool Go::undo_move() {
  if (boards[0].size() <= 1) return false;
  const Board& old = boards[0].top();
  int last_passes = passes.top();
  passes.pop();
  // don't erase superko hist if popping a pass
  if (last_passes <= passes.top())  superko_hist.erase(old.h);
  for (int i = 0; i < NUM_ISO; i++) {
    boards[i].pop();
    paths[i].pop();
    assert(passes.size() == boards[i].size());
    assert(paths[i].size() == boards[i].size());
  }
  switch_to_move();
  return true;
}

float Go::score(Color c) {
  return boards[0].top().score(c);
}

void Go::print_board() {
  boards[0].top().print();
}

void Go::switch_to_move() {
  to_move = (to_move == BLACK) ? WHITE : BLACK;
}

/**
 * Get the possible moves on the current board.
 *
 * nullptr is a valid parameter value for moves if we just care to test
 * that there are moves available
 **/
long Go::get_moves(std::vector<int> *moves) {
  if (moves != nullptr)  moves->clear();
  std::bitset<64> legal(boards[0].top().empty_points());
  for (int i = 0; i < n*n; i++) {
    if (legal.test(i)) {
      if (moves != nullptr)
        moves->push_back(i);
    }
  }

  // include pass move
  if (moves != nullptr) moves->push_back(PASS_IND);

  return legal.to_ulong();
}

long Go::get_current_path() {
  return paths[0].top();
}

std::array<long, NUM_ISO> Go::get_isomorphic_paths() {
  std::array<long, NUM_ISO> ret;
  for (int i = 0; i < NUM_ISO; i++) {
    ret[i] = paths[i].top();
  }
  return ret;
}

std::array<int, NUM_ISO> Go::get_isomorphic_moves(int move) {
  std::array<int, NUM_ISO> ret;
  for (int i = 0; i < NUM_ISO; i++) {
    ret[i] = move;
    if (move >= 0) {
      move = Board::rotation90_index_3x3[move];
      if (i % 4 == 0 && i > 0) {
        move = Board::horiz_flip_index_3x3[move];
      }
    }
  }
  return ret;
}

Board& Go::get_board() { return boards[0].top(); }

Color Go::opponent(Color c) { return Board::opponent(c); }
