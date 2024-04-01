#include <gtest/gtest.h>  // for Test, ASSERT_EQ, Message, TestPartResult

#include <vector>  // for allocator, vector

#include "dices.hpp"   // for DicePairRoll
#include "game.hpp"    // for Move, Play, Game, ScoredPlay, Game::Players
#include "player.hpp"  // for Player
#include "table.hpp"   // for HOME, GOAL, getPlayerInitialPosition

static void testPlay(const Game::Players& players, DicePairRoll dices,
                     PlayerNumber player, const Play& expectedBestPlay) {
  Game game(players);

  ScoredPlay bestPlayAndScore = game.bestPlay(player, dices);
  const Play& bestPlay = bestPlayAndScore.play;

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  for (unsigned int i = 0; i < bestPlay.size(); i++) {
    const auto& bestMove = bestPlay[i];
    const auto& expectedBestMove = expectedBestPlay[i];

    ASSERT_EQ(bestMove.player, expectedBestMove.player);
    ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
    ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
  }
}

TEST(TestGame, TestMoveToWin) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 2}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  // Create a scroll that allows to win but also to move to a worse position
  DicePairRoll roll{2, 1};
  Play expectedBestPlay = {{1, GOAL - 2, GOAL}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestMoveToWinDicesInversion) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 2}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  // Create a scroll that allows to win but also to move to a worse position
  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, GOAL - 2, GOAL}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestDontGetTooCloseToGoal) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 5}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  // Create a scroll that does not allow to get into the goal, but could take me
  // closer to it: to a worse position
  DicePairRoll roll{4, 2};
  Play expectedBestPlay = {{1, GOAL - 5, GOAL - 3}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestForcedToGetOutOfHome) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL - 5, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  // Roll whose dices add up 5, which forces me to get out from home
  DicePairRoll roll{4, 1};
  Play expectedBestPlay = {{1, HOME, getPlayerInitialPosition(1)}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestForcedToGetOutOfHomeTwice) {
  Game::Players players{Player({1, {GOAL, GOAL - 5, HOME, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{5, 5};
  const Position initialPosition = getPlayerInitialPosition(1);
  const Move outFromHomeMove{1, HOME, initialPosition};
  Play expectedBestPlay = {outFromHomeMove, outFromHomeMove};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestForcedToGetOutOfHomeAndMove) {
  Game::Players players{Player({1, {GOAL, GOAL - 1, HOME, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{6, 5};
  const Position initialPosition = getPlayerInitialPosition(1);
  Play expectedBestPlay = {{1, HOME, initialPosition},
                           {1, initialPosition, initialPosition + 6}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestMoveOnGoal) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL - 1, initialPosition, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, GOAL - 1, GOAL},
                           {1, initialPosition, initialPosition + 10},
                           {1, initialPosition + 10, initialPosition + 12}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestCannotMoveOnGoal) {
  // Once I get to goal I will win 10 advances,
  // but I have no pieces that can use them
  Game::Players players{Player({1, {GOAL, GOAL - 1, GOAL - 3, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, GOAL - 1, GOAL}, {1, GOAL - 3, GOAL - 1}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestCannotMoveAfterBoostOnGoal) {
  // I can get to goal with one dice, I can use the boost
  // but I cannot use the other dice
  Game::Players players{Player({1, {GOAL, GOAL - 1, 61, GOAL}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, GOAL - 1, GOAL}, {1, 61, GOAL - 1}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestConsecutiveGoalBoost) {
  Game::Players players{Player({1, {GOAL, GOAL - 1, 62, 58}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, GOAL - 1, GOAL},
                           {1, 62, GOAL},
                           {1, 58, GOAL - 4},
                           {1, GOAL - 4, GOAL - 2}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestGoalBoostWithFullDice) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL - 4, initialPosition, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 3};
  Play expectedBestPlay = {{1, GOAL - 4, GOAL - 3},
                           {1, GOAL - 3, GOAL},
                           {1, initialPosition, initialPosition + 10}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestEatAdversary) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL, GOAL, initialPosition}}),
                        Player({2, {HOME, HOME, HOME, initialPosition + 3}})};

  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, initialPosition, initialPosition + 1},
                           {1, initialPosition + 1, initialPosition + 3},
                           {2, initialPosition + 3, HOME},
                           {1, initialPosition + 3, initialPosition + 23}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestDontEatOnSafePosition) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL, GOAL, initialPosition}}),
                        Player({2, {GOAL, GOAL, GOAL, 8}})};

  DicePairRoll roll{2, 5};
  Play expectedBestPlay = {{1, initialPosition, initialPosition + 2},
                           {1, initialPosition + 2, initialPosition + 7}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestDontEatOnHallway) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 7}}),
                        Player({2, {GOAL, GOAL, GOAL, GOAL - 1}})};

  DicePairRoll roll{2, 4};
  Play expectedBestPlay = {{1, GOAL - 7, GOAL - 5}, {1, GOAL - 5, GOAL - 1}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestChooseToEat) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL, GOAL, initialPosition}}),
                        Player({2, {GOAL, GOAL, GOAL, 4}})};

  DicePairRoll roll{4, 3};
  Play expectedBestPlay = {
      {1, initialPosition, 4}, {2, 4, HOME}, {1, 4, 24}, {1, 24, 28}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, TestAdversaryEatsMe) {
  Game::Players players{Player({1, {GOAL, GOAL, 61, GOAL}}),
                        Player({2, {GOAL, GOAL, 59, HOME}})};

  DicePairRoll roll{4, 2};
  Play expectedBestPlay = {
      {2, 59, 61}, {1, 61, HOME}, {2, 61, 13}, {2, 13, 17}};

  testPlay(players, roll, 2, expectedBestPlay);
}

TEST(TestGame, TestBoostMixture) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, initialPosition, 62, 52}}),
                        Player({2, {GOAL, GOAL, 12, 2}})};

  DicePairRoll roll{4, 1};
  Play expectedBestPlay = {{1, initialPosition, 2},
                           {2, 2, HOME},
                           {1, 52, GOAL},
                           {1, 62, GOAL},
                           {1, 2, 12},
                           {2, 12, HOME},
                           {1, 12, 32},
                           {1, 32, 36}};

  testPlay(players, roll, 1, expectedBestPlay);
}
