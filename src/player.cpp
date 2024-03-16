#include "player.hpp"

#include <algorithm>  // for find
#include <array>      // for array<>::iterator, array
#include <sstream>    // for operator<<, ostringstream, basic_ostream, basic...

#include "dices.hpp"  // for getDiceValProbability, averageDiceRoll
#include "table.hpp"  // for Position, getPlayerInitialPosition, getPlayerLa...

static double piecePunctuation(Position piece, Position finalPosition,
                               Position initialPosition) {
  // If the piece got to the goal, there is no need of moving it
  if (piece == GOAL) return 0.0;

  double punctuation{0.0};

  // Add the average points you get before you see the first five
  if (piece == HOME) {
    punctuation += 1 / getDiceValProbability(5) * averageDiceRoll;
  }

  // There is path to move until getting to the hallway
  if (piece < firstHallway) {
    Position playingPosition = (piece == HOME) ? initialPosition : piece;
    punctuation += distanceToPosition(playingPosition, finalPosition);
  }

  // Add the average dices rolls to get to the goal from the final hallway
  unsigned int distanceToGoal =
      (piece < firstHallway) ? hallwayLength : (GOAL - piece);
  punctuation += 1 / getDiceValProbability(distanceToGoal) * averageDiceRoll;

  return punctuation;
}

double Player::punctuation() const {
  double punctuation{0.0};

  Position finalPosition{getPlayerLastPosition(playerNumber)};
  Position initialPosition{getPlayerInitialPosition(playerNumber)};
  for (Position piece : pieces) {
    punctuation += piecePunctuation(piece, finalPosition, initialPosition);
  }

  return punctuation;
}

static Position destinyPosition(Position pieceToMove,
                                unsigned int positionsToMove,
                                PlayerNumber playerNumber) {
  // If the piece is at home the only move it can make is exit
  if (pieceToMove == HOME) {
    if (positionsToMove == 5) {
      pieceToMove = getPlayerInitialPosition(playerNumber);
    } else {
      std::ostringstream oss;
      oss << "A piece at home cannot be moved with a " << positionsToMove
          << ".";
      throw Player::WrongMove(oss.str());
    }
  }
  // The piece is in a common position
  else if (isCommonPosition(pieceToMove)) {
    Position finalPosition = getPlayerLastPosition(playerNumber);
    unsigned int distanceToHallWay =
        1 + distanceToPosition(pieceToMove, finalPosition);

    // If the piece advance all the positions the dice say it does not get into
    // the hallway
    if (distanceToHallWay > positionsToMove) {
      // Correct the number if it has gone further than the position 1
      pieceToMove = correctPosition(pieceToMove + positionsToMove);
    }
    // The piece will get to the hallway. Let's check it can move that far
    else {
      unsigned int distanceToGoal = distanceToHallWay + hallwayLength;
      if (distanceToGoal < positionsToMove) {
        std::ostringstream oss;
        oss << "There is not space enough to move " << positionsToMove
            << " positions.";
        throw Player::WrongMove(oss.str());
      }
      pieceToMove = firstHallway + positionsToMove - distanceToHallWay;
    }
  }
  // The piece is in the final hallway
  else if (isHallwayPosition(pieceToMove)) {
    unsigned int distanceToGoal = GOAL - pieceToMove;
    if (distanceToGoal < positionsToMove) {
      std::ostringstream oss;
      oss << "There is not space enough to move " << positionsToMove
          << " positions.";
      throw Player::WrongMove(oss.str());
    }
    pieceToMove += positionsToMove;
  }
  // I cannot move a piece that has reached the goal
  else if (pieceToMove == GOAL) {
    throw Player::WrongMove("Piece on goal cannot be moved.");
  }

  return pieceToMove;
}

void Player::movePiece(Position pieceToMove, unsigned int positionsToMove) {
  // Check I have the pice I was asked to move
  auto itPieceToMove = std::find(pieces.begin(), pieces.end(), pieceToMove);
  if (itPieceToMove == pieces.end())
    throw PieceNotFound("No piece to be moved");
  Position& toMove = *itPieceToMove;

  toMove = destinyPosition(toMove, positionsToMove, playerNumber);
}
