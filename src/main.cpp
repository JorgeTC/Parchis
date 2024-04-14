#include <iostream>

#include "game.hpp"
#include "player.hpp"
#include "table.hpp"

void printBestPlay(const Play& play) {
  for (const Move& move : play) {
    std::cout << "Player number " << move.player << " move piece from "
              << move.origin << " to " << move.dest << "\n";
  }
}

int main() {
  Game::Players players{Player({1, {1, 34, 11, 7}}),
                        Player({2, {GOAL - 3, 47, 35, 41}})};

  DicePairRoll roll{1, 2};
  Game game(players);
  auto bestPlay = game.bestPlay(1, roll, 1, 2);
  printBestPlay(bestPlay.play);

  return 0;
}
