#include <gtest/gtest.h>  // for Test, ASSERT_EQ, Message, TestPartResult

#include <vector>  // for allocator, vector

#include "dices.hpp"   // for DicePairRoll
#include "game.hpp"    // for Move, Play, Game, ScoredPlay, Game::Players
#include "player.hpp"  // for Player
#include "table.hpp"   // for HOME, GOAL, getPlayerInitialPosition

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

TEST(TestGame, TestForcedToGetOutOfHome) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL - 5, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  // Roll whose dices add up 5, which forces me to get out from home
  DicePairRoll roll{4, 1};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  Play expectedBestPlay = {{1, HOME, getPlayerInitialPosition(1)}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  const Move& bestMove = bestPlay[0];
  const Move& expectedBestMove = expectedBestPlay[0];
  ASSERT_EQ(bestMove.player, expectedBestMove.player);
  ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
  ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
}

TEST(TestGame, TestForcedToGetOutOfHomeTwice) {
  Game::Players players{Player({1, {GOAL, GOAL - 5, HOME, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  DicePairRoll roll{5, 5};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  const Position initialPosition = getPlayerInitialPosition(1);
  const Move outFromHomeMove{1, HOME, initialPosition};

  ASSERT_EQ(bestPlay.size(), 2);
  for (const Move& bestMove : bestPlay) {
    ASSERT_EQ(bestMove.player, outFromHomeMove.player);
    ASSERT_EQ(bestMove.origin, outFromHomeMove.origin);
    ASSERT_EQ(bestMove.dest, outFromHomeMove.dest);
  }
}

TEST(TestGame, TestForcedToGetOutOfHomeAndMove) {
  Game::Players players{Player({1, {GOAL, GOAL - 1, HOME, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  DicePairRoll roll{6, 5};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  const Position initialPosition = getPlayerInitialPosition(1);
  Play expectedBestPlay = {{1, HOME, initialPosition},
                           {1, initialPosition, initialPosition + 6}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  for (unsigned int i = 0; i < bestPlay.size(); i++) {
    const auto& bestMove = bestPlay[i];
    const auto& expectedBestMove = expectedBestPlay[i];

    ASSERT_EQ(bestMove.player, expectedBestMove.player);
    ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
    ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
  }
}

TEST(TestGame, TestMoveOnGoal) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL - 1, initialPosition, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  DicePairRoll roll{1, 2};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  Play expectedBestPlay = {{1, GOAL - 1, GOAL},
                           {1, initialPosition, initialPosition + 10},
                           {1, initialPosition + 10, initialPosition + 12}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  for (unsigned int i{0}; i < bestPlay.size(); i++) {
    const auto& bestMove = bestPlay[i];
    const auto& expectedBestMove = expectedBestPlay[i];

    ASSERT_EQ(bestMove.player, expectedBestMove.player);
    ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
    ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
  }
}

TEST(TestGame, TestCannotMoveOnGoal) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL - 1, GOAL - 3, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  DicePairRoll roll{1, 2};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  Play expectedBestPlay = {{1, GOAL - 1, GOAL}, {1, GOAL - 3, GOAL - 1}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  for (unsigned int i{0}; i < bestPlay.size(); i++) {
    const auto& bestMove = bestPlay[i];
    const auto& expectedBestMove = expectedBestPlay[i];

    ASSERT_EQ(bestMove.player, expectedBestMove.player);
    ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
    ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
  }
}

TEST(TestGame, TestCannotMoveAfterBoostOnGoal) {
  Game::Players players{Player({1, {GOAL, GOAL - 1, 61, GOAL}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  DicePairRoll roll{1, 2};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  Play expectedBestPlay = {{1, GOAL - 1, GOAL}, {1, 61, GOAL - 1}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  for (unsigned int i{0}; i < bestPlay.size(); i++) {
    const auto& bestMove = bestPlay[i];
    const auto& expectedBestMove = expectedBestPlay[i];

    ASSERT_EQ(bestMove.player, expectedBestMove.player);
    ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
    ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
  }
}

TEST(TestGame, TestConsecutiveGoalBoost) {
  Game::Players players{Player({1, {GOAL, GOAL - 1, 62, 58}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  Game game(players);

  DicePairRoll roll{1, 2};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  Play expectedBestPlay = {{1, GOAL - 1, GOAL},
                           {1, 62, GOAL},
                           {1, 58, GOAL - 4},
                           {1, GOAL - 4, GOAL - 2}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  for (unsigned int i{0}; i < bestPlay.size(); i++) {
    const auto& bestMove = bestPlay[i];
    const auto& expectedBestMove = expectedBestPlay[i];

    ASSERT_EQ(bestMove.player, expectedBestMove.player);
    ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
    ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
  }
}

TEST(TestGame, TestEatAdversary) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL, GOAL, initialPosition}}),
                        Player({2, {HOME, HOME, HOME, initialPosition + 3}})};

  Game game(players);

  DicePairRoll roll{1, 2};
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;
  Play expectedBestPlay = {{1, initialPosition, initialPosition + 1},
                           {1, initialPosition + 1, initialPosition + 2},
                           {2, initialPosition + 2, HOME},
                           {1, initialPosition + 2, initialPosition + 22}};

  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  for (unsigned int i{0}; i < bestPlay.size(); i++) {
    const auto& bestMove = bestPlay[i];
    const auto& expectedBestMove = expectedBestPlay[i];

    ASSERT_EQ(bestMove.player, expectedBestMove.player);
    ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
    ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
  }
}