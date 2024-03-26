#include <gtest/gtest.h>  // for Test, ASSERT_EQ, Message, TestPartResult

#include <memory>  // for allocator

#include "dices.hpp"   // for DicePairRoll
#include "game.hpp"    // for Move, Play, Game, ScoredPlay, Game::Players
#include "player.hpp"  // for Player
#include "table.hpp"   // for GOAL, HOME

TEST(TestGame, TestMoveToWin) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 2}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  // Create a scroll that allows to win but also to move to a worse position
  DicePairRoll roll{2, 1};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  Play expectedBestPlay = {{1, GOAL - 2, GOAL}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  const Move& bestMove = bestPlay[0];
  const Move& expectedBestMove = expectedBestPlay[0];
  ASSERT_EQ(bestMove.player, expectedBestMove.player);
  ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
  ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
}

TEST(TestGame, TestMoveToWinDicesInversion) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 2}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  // Create a scroll that allows to win but also to move to a worse position
  DicePairRoll roll{1, 2};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  Play expectedBestPlay = {{1, GOAL - 2, GOAL}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  const Move& bestMove = bestPlay[0];
  const Move& expectedBestMove = expectedBestPlay[0];
  ASSERT_EQ(bestMove.player, expectedBestMove.player);
  ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
  ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
}

TEST(TestGame, TestDontGetTooCloseToGoal) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 5}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  // Create a scroll that does not allow to get into the goal, but could take me
  // closer to it: to a worse position
  DicePairRoll roll{4, 2};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  Play expectedBestPlay = {{1, GOAL - 5, GOAL - 3}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  const Move& bestMove = bestPlay[0];
  const Move& expectedBestMove = expectedBestPlay[0];
  ASSERT_EQ(bestMove.player, expectedBestMove.player);
  ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
  ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
}
