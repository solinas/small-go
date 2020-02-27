// Copyright 2019 Chris Solinas
#pragma once
#include <stack>
#include <set>
#include <vector>

#include "board.h"


/*
 * Play games of Go using a fast bitboard implementation
 *
 * Uses the least significant bits for boards smaller than 8x8.
 * The bottom right corner of the board is always the least significant
 * digit

 * */

constexpr int PASS_IND = -1;
constexpr int MAX_VAL = 100000;
constexpr int NUM_ISO = 8;

class Go {
  std::vector<std::stack<Board>> boards;
  std::set<long> superko_hist;
  int to_move;
  int n;
  std::stack<int> passes;

  // private methods
  void switch_to_move();

 public:
  Go(int n);
  ~Go();

  bool make_move(int point_ind, Color color);
  bool undo_move();
  float score(Color color);
  long get_moves(std::vector<int> *moves);
  long get_pruned_moves(std::vector<int> *moves);
  void print_board();
  int size();
  bool game_over();
  static Color opponent(Color c);
  Board& get_board();
};


