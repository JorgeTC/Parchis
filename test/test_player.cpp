#include <gtest/gtest.h>

#include <stdexcept>

#include "player.hpp"
#include "table.hpp"
#include "dices.hpp"

TEST(TestPlayer, TestPunctuation) {
  Player player1{1, {HOME, HOME, HOME, HOME}};
  double punctuation1 = player1.punctuation();

  Player player2{2, {HOME, HOME, HOME, HOME}};
  double punctuation2 = player2.punctuation();

  ASSERT_DOUBLE_EQ(punctuation1, punctuation2);
}

TEST(TestPlayer, TestPunctuationAtInitialPosition) {
  Position player1Start = getPlayerInitialPosition(1);
  Player player1{1, {player1Start, HOME, HOME, HOME}};
  double punctuation1 = player1.punctuation();

  Position player2Start = getPlayerInitialPosition(2);
  Player player2{2, {player2Start, HOME, HOME, HOME}};
  double punctuation2 = player2.punctuation();

  ASSERT_DOUBLE_EQ(punctuation1, punctuation2);
}

TEST(TestPlayer, TestWonPunctuation) {
  // Set all pieces on the goal
  Player player1{1, {GOAL, GOAL, GOAL, GOAL}};
  double punctuation1 = player1.punctuation();

  ASSERT_DOUBLE_EQ(punctuation1, 0.0);

  Player player2{2, {GOAL, GOAL, GOAL, GOAL}};
  double punctuation2 = player2.punctuation();

  ASSERT_DOUBLE_EQ(punctuation2, 0.0);
}

TEST(TestPlayer, TestPunctuationOnePositionToGet) {
  Player player1{1, {GOAL, GOAL, GOAL, finalHallway}};
  double punctuation1 = player1.punctuation();

  double constexpr expectedPoints = averageDiceRoll / getDiceValProbability(1);
  ASSERT_DOUBLE_EQ(punctuation1, expectedPoints);
}

TEST(TestPlayer, TestMoveToWin) {
  // Place a piece at distance 10 to goal
  {
    Player player{1, {HOME, HOME, HOME, 62}};
    player.movePiece(62, 10);

    std::array<Position, 4> expectedPositions({HOME, HOME, HOME, GOAL});
    ASSERT_EQ(player.pieces, expectedPositions);
  }
  {
    Player player{2, {HOME, HOME, HOME, 28}};
    player.movePiece(28, 10);

    std::array<Position, 4> expectedPositions({HOME, HOME, HOME, GOAL});
    ASSERT_EQ(player.pieces, expectedPositions);
  }
}

TEST(TestPlayer, TestMoveTooMuch) {
  // Place a piece at distance 10 to goal
  {
    Player player{1, {HOME, HOME, HOME, 62}};
    EXPECT_THROW(player.movePiece(62, 15), Player::WrongMove);
  }
  {
    Player player{2, {HOME, HOME, HOME, 28}};
    EXPECT_THROW(player.movePiece(28, 15), Player::WrongMove);
  }
}

TEST(TestPlayer, TestMoveHome) {
  for (PlayerNumber number = 1; number <= 2; number++) {
    Player player{number, {HOME, HOME, HOME, HOME}};
    player.movePiece(HOME, 5);
    Position playerStart = getPlayerInitialPosition(number);
    std::array<Position, 4> expectedPositions({playerStart, HOME, HOME, HOME});
    ASSERT_EQ(player.pieces, expectedPositions);
  }
}

TEST(TestPlayer, TestErrorNonExistingPosition) {
  Player player{1, {1, 2, 3, 4}};

  EXPECT_THROW(player.movePiece(HOME, 5), Player::PieceNotFound);
  EXPECT_THROW(player.movePiece(5, 1), Player::PieceNotFound);
  EXPECT_THROW(player.movePiece(firstHallway, 1), Player::PieceNotFound);
  EXPECT_THROW(player.movePiece(GOAL, 1), Player::PieceNotFound);
}

TEST(TestPlayer, TestErrorMovingGoal) {
  Player player{1, {HOME, HOME, HOME, GOAL}};

  EXPECT_THROW(player.movePiece(GOAL, 5), Player::WrongMove);
  EXPECT_THROW(player.movePiece(GOAL, 1), Player::WrongMove);
  EXPECT_THROW(player.movePiece(GOAL, 10), Player::WrongMove);
}

TEST(TestPlayer, TestWrongNumberForHome) {
  Player player{1, {HOME, HOME, HOME, HOME}};

  EXPECT_THROW(player.movePiece(HOME, 1), Player::WrongMove);
  EXPECT_THROW(player.movePiece(HOME, 10), Player::WrongMove);
}

TEST(TestPlayer, TestMoveFromHallway) {
  Player player{1, {firstHallway + 2, HOME, HOME, HOME}};

  player.movePiece(firstHallway + 2, 3);
  std::array<Position, 4> expectedPositions(
      {firstHallway + 5, HOME, HOME, HOME});
  ASSERT_EQ(player.pieces, expectedPositions);
}

TEST(TestPlayer, TestErrorTooMuchMoveFromHallway) {
  Player player{1, {firstHallway + 2, HOME, HOME, HOME}};

  EXPECT_THROW(player.movePiece(firstHallway + 2, 10), Player::WrongMove);
}

TEST(TestPlayer, TestWinFromHallway) {
  Player player{1, {finalHallway, HOME, HOME, HOME}};

  player.movePiece(finalHallway, 1);
  std::array<Position, 4> expectedPositions({GOAL, HOME, HOME, HOME});
  ASSERT_EQ(player.pieces, expectedPositions);
}

TEST(TestPlayer, TestFurtherThanOne) {
  {
    Player player{2, {63, HOME, HOME, HOME}};

    player.movePiece(63, 7);
    std::array<Position, 4> expectedPositions({2, HOME, HOME, HOME});
    ASSERT_EQ(player.pieces, expectedPositions);
  }
  {
    Player player{2, {65, HOME, HOME, HOME}};

    player.movePiece(65, 7);
    std::array<Position, 4> expectedPositions({4, HOME, HOME, HOME});
    ASSERT_EQ(player.pieces, expectedPositions);
  }
  {
    Player player{2, {65, HOME, HOME, HOME}};

    player.movePiece(65, 34);
    std::array<Position, 4> expectedPositions({firstHallway, HOME, HOME, HOME});
    ASSERT_EQ(player.pieces, expectedPositions);
  }
  {
    Player player{2, {65, HOME, HOME, HOME}};

    player.movePiece(65, 41);
    std::array<Position, 4> expectedPositions({GOAL, HOME, HOME, HOME});
    ASSERT_EQ(player.pieces, expectedPositions);
  }
}