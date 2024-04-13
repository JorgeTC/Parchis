#include <gtest/gtest.h>  // for Test, SuiteApiResolver, TestInfo (ptr only)

#include <algorithm>  // for count
#include <array>      // for array
#include <memory>     // for allocator_traits<>::value_type
#include <set>        // for operator==, set
#include <string>     // for allocator, string
#include <vector>     // for vector

#include "dices.hpp"   // for DicePairRoll
#include "game.hpp"    // for Play, Game, Game::Players, Move, ScoredPlay
#include "player.hpp"  // for Player
#include "table.hpp"   // for GOAL, HOME, getPlayerInitialPosition, Position

static void compareMove(const Move& bestMove, const Move& expectedBestMove) {
  ASSERT_EQ(bestMove.player, expectedBestMove.player);
  ASSERT_EQ(bestMove.origin, expectedBestMove.origin);
  ASSERT_EQ(bestMove.dest, expectedBestMove.dest);
}

static void comparePlays(const Play& bestPlay, const Play& expectedBestPlay) {
  ASSERT_EQ(bestPlay.size(), expectedBestPlay.size());
  for (unsigned int i = 0; i < bestPlay.size(); i++) {
    const Move& bestMove = bestPlay[i];
    const Move& expectedBestMove = expectedBestPlay[i];

    compareMove(bestMove, expectedBestMove);
  }
}

static void testPlay(const Game::Players& players, DicePairRoll dices,
                     PlayerNumber player, const Play& expectedBestPlay) {
  Game game(players);

  ScoredPlay bestPlayAndScore = game.bestPlay(player, dices);
  const Play& bestPlay = bestPlayAndScore.play;

  comparePlays(bestPlay, expectedBestPlay);
}

static Game getFinalState(const Game::Players& players, DicePairRoll dices,
                          PlayerNumber player) {
  Game game(players);

  ScoredPlay bestPlayAndScore = game.bestPlay(player, dices);
  const Play& bestPlay = bestPlayAndScore.play;

  for (const Move& movement : bestPlay) {
    game.takePiece(movement.player, movement.origin, movement.dest);
  }

  return game;
}

TEST(TestGame, MoveToWin) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 2}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  // Create a scroll that allows to win but also to move to a worse position
  DicePairRoll roll{2, 1};
  Play expectedBestPlay = {{1, GOAL - 2, GOAL}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, MoveToWinDicesInversion) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 2}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  // Create a scroll that allows to win but also to move to a worse position
  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, GOAL - 2, GOAL}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, DontGetTooCloseToGoal) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 5}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  // Create a scroll that does not allow to get into the goal, but could take me
  // closer to it: to a worse position
  DicePairRoll roll{4, 2};
  Play expectedBestPlay = {{1, GOAL - 5, GOAL - 3}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, ForcedToGetOutOfHome) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL - 5, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  // Roll whose dices add up 5, which forces me to get out from home
  DicePairRoll roll{4, 1};
  Play expectedBestPlay = {{1, HOME, getPlayerInitialPosition(1)}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, ForcedToGetOutOfHomeTwice) {
  Game::Players players{Player({1, {GOAL, GOAL - 5, HOME, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{5, 5};
  const Position initialPosition = getPlayerInitialPosition(1);
  const Move outFromHomeMove{1, HOME, initialPosition};
  Play expectedBestPlay = {outFromHomeMove, outFromHomeMove};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, ForcedToGetOutOfHomeAndMove) {
  Game::Players players{Player({1, {GOAL, GOAL - 1, HOME, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{6, 5};
  const Position initialPosition = getPlayerInitialPosition(1);
  Play expectedBestPlay = {{1, HOME, initialPosition},
                           {1, initialPosition, initialPosition + 6}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, MoveOnGoal) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL - 1, initialPosition, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, GOAL - 1, GOAL},
                           {1, initialPosition, initialPosition + 10},
                           {1, initialPosition + 10, initialPosition + 12}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, CannotMoveOnHOME) {
  // Player without playable pieces
  Game::Players players{Player({1, {GOAL, GOAL, HOME, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{4, 2};

  // No moves can be done
  Play expectedBestPlay = {};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, CannotMoveOnHallway) {
  // Player with playable pieces
  Game::Players players{Player({1, {GOAL, GOAL - 2, GOAL - 1, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  // Numbers too big to move the available pieces
  DicePairRoll roll{4, 3};

  // No moves can be done
  Play expectedBestPlay = {};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, CannotMoveOnGoal) {
  // Once I get to goal I will win 10 advances,
  // but I have no pieces that can use them
  Game::Players players{Player({1, {GOAL, GOAL - 1, GOAL - 3, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 2};

  Game finalState = getFinalState(players, roll, 1);

  const Player& mover = finalState.getPlayer(1);

  // Check I put one piece on the goal which means I got my impossible boost
  unsigned int piecesAtGoal =
      std::count(mover.pieces.begin(), mover.pieces.end(), GOAL);
  ASSERT_TRUE(piecesAtGoal == 2);

  unsigned int piecesAtHome =
      std::count(mover.pieces.begin(), mover.pieces.end(), HOME);
  ASSERT_TRUE(piecesAtHome == 1);
}

TEST(TestGame, CannotMoveAfterBoostOnGoal) {
  // I can get to goal with one dice, I can use the boost
  // but I cannot use the other dice
  Game::Players players{Player({1, {GOAL, GOAL - 1, 61, GOAL}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, GOAL - 1, GOAL}, {1, 61, GOAL - 1}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, ConsecutiveGoalBoost) {
  Game::Players players{Player({1, {GOAL, GOAL - 1, 62, 58}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 2};
  Play expectedBestPlay = {{1, GOAL - 1, GOAL},
                           {1, 62, GOAL},
                           {1, 58, GOAL - 4},
                           {1, GOAL - 4, GOAL - 2}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, GoalBoostWithFullDice) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL - 4, initialPosition, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{1, 3};
  Play expectedBestPlay = {{1, GOAL - 4, GOAL - 3},
                           {1, GOAL - 3, GOAL},
                           {1, initialPosition, initialPosition + 10}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, EatAdversary) {
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

TEST(TestGame, DontEatOnSafePosition) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL, GOAL, initialPosition}}),
                        Player({2, {GOAL, GOAL, GOAL, 8}})};

  DicePairRoll roll{2, 5};
  Play expectedBestPlay = {{1, initialPosition, initialPosition + 2},
                           {1, initialPosition + 2, initialPosition + 7}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, DontEatOnHallway) {
  Game::Players players{Player({1, {GOAL, GOAL, GOAL, GOAL - 7}}),
                        Player({2, {GOAL, GOAL, GOAL, GOAL - 1}})};

  DicePairRoll roll{2, 4};
  Play expectedBestPlay = {{1, GOAL - 7, GOAL - 5}, {1, GOAL - 5, GOAL - 1}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, ChooseToEat) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{Player({1, {GOAL, GOAL, GOAL, initialPosition}}),
                        Player({2, {GOAL, GOAL, GOAL, 4}})};

  DicePairRoll roll{4, 3};
  Play expectedBestPlay = {
      {1, initialPosition, 4}, {2, 4, HOME}, {1, 4, 24}, {1, 24, 28}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, AdversaryEatsMe) {
  Game::Players players{Player({1, {GOAL, GOAL, 61, GOAL}}),
                        Player({2, {GOAL, GOAL, 59, HOME}})};

  DicePairRoll roll{4, 2};
  Play expectedBestPlay = {
      {2, 59, 61}, {1, 61, HOME}, {2, 61, 13}, {2, 13, 17}};

  testPlay(players, roll, 2, expectedBestPlay);
}

TEST(TestGame, BoostMixture) {
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

TEST(TestGame, LoadBarrierSamePlayer) {
  Position initialPosition = getPlayerInitialPosition(1);

  // Create a combination in which player 1 could eat player 2
  // if it was not for the barrier
  Game::Players players{Player({1, {GOAL, initialPosition, 61, GOAL}}),
                        Player({2, {GOAL, GOAL, 62, 62}})};

  Game game(players);

  ASSERT_TRUE(game.barriers == std::set<Position>{62});
}

TEST(TestGame, LoadBarrierDifferentPlayers) {
  Game::Players players{Player({1, {GOAL, 1, 61, GOAL - 1}}),
                        Player({2, {GOAL, 1, 62, GOAL - 1}})};

  Game game(players);

  ASSERT_TRUE(game.barriers == std::set<Position>{1});
}

TEST(TestGame, NotBarrierOnHallway) {
  // Create a combination in which player 1 could eat player 2
  // if it was not for the barrier
  Game::Players players{Player({1, {HOME, HOME, GOAL - 1, GOAL - 1}}),
                        Player({2, {HOME, HOME, 62, 62}})};

  Game game(players);

  ASSERT_TRUE(game.barriers == std::set<Position>{62});
}

TEST(TestGame, DontCrossBarrier) {
  Position initialPosition = getPlayerInitialPosition(1);

  // Create a combination in which player 1 could eat player 2
  // if it was not for the barrier
  Game::Players players{Player({1, {GOAL, initialPosition, 61, GOAL}}),
                        Player({2, {GOAL, GOAL, 62, 62}})};

  DicePairRoll roll{4, 1};
  Play expectedBestPlay = {{1, initialPosition, initialPosition + 4},
                           {1, initialPosition + 4, initialPosition + 5}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, DontGoOutIfBarrier) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{
      Player({1, {GOAL, initialPosition, initialPosition, HOME}}),
      Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{4, 1};

  Game finalState = getFinalState(players, roll, 1);
  const Player& mover = finalState.getPlayer(1);

  // Check I put one piece on the goal which means I got my impossible boost
  unsigned int piecesAtGoal =
      std::count(mover.pieces.begin(), mover.pieces.end(), GOAL);
  ASSERT_TRUE(piecesAtGoal == 1);

  // I cannot check the exact number of pieces on the init position.
  // I only know at least one of them has been moved
  unsigned int piecesAtInit =
      std::count(mover.pieces.begin(), mover.pieces.end(), initialPosition);
  ASSERT_TRUE(piecesAtInit < 2);

  // Check the one at hme remains at home
  unsigned int piecesAtHome =
      std::count(mover.pieces.begin(), mover.pieces.end(), HOME);
  ASSERT_TRUE(piecesAtHome == 1);
}

TEST(TestGame, GoOutAfterRemoveBarrier) {
  Position initialPosition = getPlayerInitialPosition(1);

  Game::Players players{
      Player({1, {GOAL, initialPosition, initialPosition, HOME}}),
      Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{5, 1};
  Play expectedBestPlay = {{1, initialPosition, initialPosition + 1},
                           {1, HOME, initialPosition}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, CreateBarrierAfterMove) {
  Game::Players players{Player({1, {GOAL, GOAL, 1, HOME}}),
                        Player({2, {HOME, HOME, 8, HOME}})};

  DicePairRoll roll{4, 3};
  Play expectedBestPlay = {{1, 1, 5}, {1, 5, 8}};

  Game game(players);
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;

  comparePlays(bestPlay, expectedBestPlay);

  for (const Move& move : bestPlay) {
    unsigned int advance = move.dest - move.origin;
    game.movePiece(move.player, move.origin, advance);
  }

  ASSERT_TRUE(game.barriers == std::set<Position>{8});
}

TEST(TestGame, CannotMoveBecauseBarrier) {
  // Two playable pieces that cannot be moved because of a barrier
  Game::Players players{Player({1, {GOAL, 2, 1, HOME}}),
                        Player({2, {HOME, 3, 3, HOME}})};

  DicePairRoll roll{4, 6};
  Play expectedBestPlay = {};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, EatOnExit) {
  Game::Players players{Player({1, {GOAL, GOAL, 1, HOME}}),
                        Player({2, {HOME, HOME, 1, HOME}})};

  DicePairRoll roll{2, 3};

  Play expectedBestPlay = {{1, HOME, 1}, {2, 1, HOME}, {1, 1, 21}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, NotEatOnExit) {
  Game::Players players{Player({1, {GOAL, GOAL, HOME, HOME}}),
                        Player({2, {HOME, HOME, 1, HOME}})};

  DicePairRoll roll{2, 3};

  Play expectedBestPlay = {{1, HOME, 1}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, EnemyBarrierOnExit) {
  Game::Players players{Player({1, {GOAL, GOAL, HOME, HOME}}),
                        Player({2, {HOME, HOME, 1, 1}})};

  DicePairRoll roll{5, 5};

  Play expectedBestPlay = {
      {1, HOME, 1}, {2, 1, HOME}, {1, 1, 21}, {1, HOME, 1}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, ExitAfterBreakingBarrierWithBoost) {
  Game::Players players{Player({1, {59, 1, 1, HOME}}),
                        Player({2, {HOME, HOME, HOME, 60}})};

  DicePairRoll roll{1, 5};

  Play expectedBestPlay = {
      {1, 59, 60}, {2, 60, HOME}, {1, 1, 21}, {1, HOME, 1}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, BreakBarrierOnDouble) {
  // Place a piece on a position that would let me insert one piece
  // Instead of choosing that option, the barrier must be broken
  Game::Players players{Player({1, {GOAL - 5, 1, 1, HOME}}),
                        Player({2, {HOME, HOME, HOME, HOME}})};

  DicePairRoll roll{5, 5};

  Play expectedBestPlay = {{1, 1, 6}, {1, HOME, 1}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, BreakTheOnlyPossibleBarrier) {
  // Player 1 has two barriers: in position 42 and in position 7.
  // There is another barrier on 47 so barrier on 42 cannot be broken with a 6.
  // The barrier on 7 must be broken.
  // The other dice must be used by piece on 5 or on 13 and cannot
  // be used by the other position on 7 to not form another barrier.
  Game::Players players{Player({1, {42, 7, 5, 7}}),
                        Player({2, {42, 47, 47, GOAL}})};

  DicePairRoll roll{6, 6};

  std::vector<Play> expectedBestPlays = {{{1, 7, 13}, {1, 5, 11}},
                                         {{1, 7, 13}, {1, 13, 19}}};

  Game game(players);
  auto mover = game.getPlayer(1);
  std::vector<Game::Turn> states = game.allPossibleStates(mover, roll);

  // Check there are two possibilities
  ASSERT_EQ(states.size(), expectedBestPlays.size());

  for (unsigned int i = 0; i < states.size(); i++) {
    comparePlays(states[i].movements, expectedBestPlays[i]);
  }
}

TEST(TestGame, BreakSecondBarrierWithBoost) {
  Game::Players players{Player({1, {2, 2, 46, 46}}),
                        Player({2, {3, GOAL, 13, 13}})};

  DicePairRoll roll{1, 1};

  Play expectedFirstMoves = {{1, 2, 3}, {2, 3, HOME}, {1, 46, GOAL - 6}};

  Game game(players);
  ScoredPlay bestPlayAndScore = game.bestPlay(1, roll);
  const Play& bestPlay = bestPlayAndScore.play;

  ASSERT_LT(expectedFirstMoves.size(), bestPlay.size());

  for (unsigned int i = 0; i < expectedFirstMoves.size(); i++) {
    compareMove(expectedFirstMoves[i], bestPlay[i]);
  }
}

TEST(TestGame, MustMoveBarrier) {
  Game::Players players{Player({1, {1, 1, GOAL, GOAL}}),
                        Player({2, {3, 3, GOAL, GOAL}})};

  DicePairRoll roll{1, 1};

  Play expectedBestPlay = {{1, 1, 2}, {1, 1, 2}};

  testPlay(players, roll, 1, expectedBestPlay);
}

TEST(TestGame, BreakBarrierAndCreateNew) {
  Game::Players players{Player({1, {5, 5, 6, GOAL}}),
                        Player({2, {8, 9, 9, GOAL}})};

  DicePairRoll roll{3, 3};

  Game game(players);
  auto mover = game.getPlayer(1);
  std::vector<Game::Turn> states = game.allPossibleStates(mover, roll);

  // Move the piece on 5 to break a barrier
  // This will create a barrier on position 8
  // The other piece on 5 cannot be moved and the piece on 8 cannot be moved
  // because the enemy has a barrier on 9
  std::vector<Play> expectedPlays = {{{1, 5, 8}}};

  ASSERT_EQ(states.size(), expectedPlays.size());
  comparePlays(states.front().movements, expectedPlays.front());
}

TEST(TestGame, NoGoBackHomeOnThirdDouble) {
  // Place pieces of 1 in positions that cannot go back home
  Game::Players players{Player({1, {HOME, GOAL - 2, HOME, GOAL}}),
                        Player({2, {8, 9, 9, GOAL}})};

  // Create a double dice
  DicePairRoll roll{3, 3};
  // Third roll
  unsigned int rollsInARow = 3;

  Game game(players);
  auto mover = game.getPlayer(1);
  // Set last touched piece
  game.setLastTouched(1, GOAL - 2);
  std::vector<Game::Turn> states =
      game.allPossibleStates(mover, roll, rollsInARow);

  std::vector<Play> expectedPlays = {{}};

  ASSERT_EQ(states.size(), expectedPlays.size());
  comparePlays(states.front().movements, expectedPlays.front());
}

TEST(TestGame, ErrorOnWrongSetLastTouched) {
  Game::Players players{Player({1, {HOME, GOAL - 2, 1, GOAL}}),
                        Player({2, {HOME, HOME, HOME, GOAL}})};

  Game game(players);

  // Try to set a piece that does not exist
  EXPECT_THROW(game.setLastTouched(1, 15), Player::PieceNotFound);
}

TEST(TestGame, GoBackHomeOnThirdDoubleSafePosition) {
  // Place only one position that can go back home
  Game::Players players{Player({1, {HOME, 7, HOME, GOAL}}),
                        Player({2, {8, HOME, GOAL, GOAL}})};

  // Create a double dice
  DicePairRoll roll{3, 3};
  // Third roll
  unsigned int rollsInARow = 3;

  Game game(players);
  auto mover = game.getPlayer(1);
  // Set last touched piece
  game.setLastTouched(1, 7);
  std::vector<Game::Turn> states =
      game.allPossibleStates(mover, roll, rollsInARow);

  std::vector<Play> expectedPlays = {{{1, 7, HOME}}};

  ASSERT_EQ(states.size(), expectedPlays.size());
  const Game::Turn& state = states.front();
  comparePlays(state.movements, expectedPlays.front());

  Position player1LastTouched = HOME;
  ASSERT_EQ(state.finalState.lastTouched[0], player1LastTouched);
}

TEST(TestGame, LastTouchedRecord) {
  // Place pieces of 1 in positions that cannot go back home
  Game::Players players{Player({1, {HOME, 7, HOME, GOAL}}),
                        Player({2, {HOME, HOME, GOAL, GOAL}})};

  Game game(players);
  game.movePiece(1, 7, 6);
  Position lastTouched = game.getLastTouched(1);
  ASSERT_EQ(lastTouched, 13);
}

TEST(TestGame, LastTouchedInTurn) {
  // Place pieces of 1 in positions that cannot go back home
  Game::Players players{Player({1, {HOME, 7, HOME, GOAL}}),
                        Player({2, {HOME, HOME, GOAL, GOAL}})};

  DicePairRoll roll{1, 2};

  Game game(players);
  auto mover = game.getPlayer(1);

  std::vector<Game::Turn> states = game.allPossibleStates(mover, roll);
  for (const Game::Turn& state : states)
    ASSERT_EQ(state.finalState.lastTouched[0], 10);
}