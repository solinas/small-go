// Copyright 2019 Chris Solinas
#include "Go.h"

#include <iostream>
#include <cassert>

Go::Go(int _n) : to_move(BLACK), n(_n) {
  Board::init_zobrist();
  
  // set up 8 boards for handling isomorphisms
  for (int i = 0; i < 8; i++) {
    std::stack<Board> b;
    b.push(Board(_n));
    boards.push_back(b);
  }
  
  passes.push(0);
}

Go::~Go() {}

int Go::size() { return n; }

bool Go::game_over() {
  return passes.top() > 1;
}

bool Go::make_move(int point_ind, Color color) {

  // first copy
  for (int i = 0; i < NUM_ISO; i++) {
    boards[i].push(boards[i].top());
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
  } else {
    // move succeeded, check superko
    if (superko_hist.find(boards[0].top().h) != superko_hist.end()) {
      res = false;
      boards[0].pop();
    } else {
      superko_hist.insert(boards[0].top().h);
    }
  }

  if (res) {
    // all checks done, reset pass counter, make moves on iso boards
    passes.push(0);
     
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
    assert(passes.size() == boards[i].size());
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

Board& Go::get_board() { return boards[0].top(); }

Color Go::opponent(Color c) { return Board::opponent(c); }
