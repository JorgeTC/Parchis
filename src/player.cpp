#include "player.hpp"

#include <algorithm>  // for find, all_of
#include <array>      // for array, array<>::const_iterator, array<>::iterator
#include <set>        // for set, operator==, set<>::const_iterator
#include <sstream>    // for operator<<, ostringstream, basic_ostream, basic...

#include "dices.hpp"  // for getDiceValProbability, OUT_OF_HOME, averageDice...
#include "table.hpp"  // for Position, getPlayerInitialPosition, getPlayerLa...

static double piecePunctuation(Position piece, Position finalPosition,
                               Position initialPosition) {
  // If the piece got to the goal, there is no need of moving it
  if (piece == GOAL) return 0.0;

  double punctuation{0.0};

  // Add the average points you get before you see the first five
  if (piece == HOME) {
    punctuation += 1 / getDiceValProbability(OUT_OF_HOME) * averageDiceRoll;
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

bool Player::hasTwoPiecesInPosition(Position targetPosition) const {
  bool seen = false;
  for (Position piece : pieces) {
    if (piece != targetPosition) continue;
    // If this is the second time i see this position, exit the function
    if (seen) return true;
    // Save that I have seen it once
    seen = true;
  }

  // I have not seen the position twice
  return false;
}

static Position destinyPosition(Position pieceToMove,
                                unsigned int positionsToMove,
                                PlayerNumber playerNumber) {
  // If the piece is at home the only move it can make is exit
  if (pieceToMove == HOME) {
    if (positionsToMove == OUT_OF_HOME) {
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

static bool existBarriersBetweenPositions(Position origin, Position destiny,
                                          const std::set<Position>& barriers) {
  // Tests the range (origin, destiny]
  return barriers.upper_bound(origin) != barriers.upper_bound(destiny);
}

static bool existBlockingBarriers(Position origin, Position destiny,
                                  const std::set<Position>& barriers) {
  // If there are not barriers at all, exit the function
  if (barriers.empty()) return false;

  // I expect the destiny position to be right
  // Just have to check there are no barriers on the initial position
  if (origin == HOME) {
    std::ostringstream oss;
    oss << "This function only checks movements across the table. "
        << "Don't call it to exit from home.";

    throw std::invalid_argument(oss.str());
  }

  // There are not barriers in the hallway
  if (isHallwayPosition(origin)) return false;

  // Origin is regular position, I have to check there are no barriers ahead

  // Case where I have not cross the position number 1
  if (origin < destiny) {
    return existBarriersBetweenPositions(origin, destiny, barriers);
  } else {
    // Check two segments and also the position number 1
    return existBarriersBetweenPositions(origin, totalPositions, barriers) ||
           barriers.find(1) != barriers.end() ||
           existBarriersBetweenPositions(1, destiny, barriers);
  }
}

bool Player::canGoToInitialPosition() const {
  // The only reason I cannot go to the first position is if there are more than
  // two pieces of mine
  Position initialPosition = getPlayerInitialPosition(playerNumber);
  return !hasTwoPiecesInPosition(initialPosition);
}

Position Player::movePiece(Position pieceToMove, unsigned int positionsToMove,
                           const std::set<Position>& barriers) {
  // Check I have the piece I was asked to move
  auto itPieceToMove = std::find(pieces.begin(), pieces.end(), pieceToMove);
  if (itPieceToMove == pieces.end())
    throw PieceNotFound("No piece to be moved");
  Position& toMove = *itPieceToMove;

  Position destiny = destinyPosition(toMove, positionsToMove, playerNumber);

  // Check the movement can be performed
  if (pieceToMove == HOME) {
    if (!canGoToInitialPosition()) {
      std::ostringstream oss;
      oss << "Initial position is too busy for me to exit." << destiny << ".";
      throw Player::WrongMove(oss.str());
    }
  } else {
    if (existBlockingBarriers(toMove, destiny, barriers)) {
      std::ostringstream oss;
      oss << "There are barriers that don't allow to move " << toMove << " to "
          << destiny << ".";
      throw Player::WrongMove(oss.str());
    }
  }

  // Execute the movement
  toMove = destiny;
  // Return the final position of the piece
  return toMove;
}

Position Player::movePiece(Position pieceToMove, unsigned int positionsToMove) {
  return movePiece(pieceToMove, positionsToMove, {});
}

void Player::pieceEaten(Position eatenPiece) {
  // Check I have the pice I was asked to move
  auto itPieceToMove = std::find(pieces.begin(), pieces.end(), eatenPiece);
  if (itPieceToMove == pieces.end())
    throw PieceNotFound("No piece to be moved");

  Position& toMove = *itPieceToMove;
  toMove = HOME;
}

bool Player::hasWon() const {
  return std::all_of(pieces.begin(), pieces.end(),
                     [](Position piece) { return piece == GOAL; });
};

std::vector<unsigned int> Player::indicesForHomePieces() const {
  std::vector<unsigned int> indices;
  indices.reserve(4);

  for (unsigned int i = 0; i < pieces.size(); i++) {
    if (pieces[i] == HOME) indices.push_back(i);
  }

  return indices;
}
