#pragma once

#include <array>      // for array
#include <set>        // for set
#include <stdexcept>  // for invalid_argument
#include <string>     // for string
#include <vector>     // for vector

#include "table.hpp"  // for HOME, Position, PlayerNumber

class Player {
 public:
  using Pieces = std::array<Position, 4>;

  double punctuation() const;

  // Checks whether all the pieces are on the goal
  bool hasWon() const;

  // Moves the piece to the returned position
  Position movePiece(Position pieceToMove, unsigned int positionsToMove);
  Position movePiece(Position pieceToMove, unsigned int positionsToMove,
                     const std::set<Position>& barriers);

  unsigned int countPiecesInPosition(Position targetPosition) const;
  bool canGoToInitialPosition() const;

  // Moves the piece to home position
  void pieceEaten(Position eatenPiece);

  struct PieceNotFound : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
  };
  struct WrongMove : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
  };

  std::vector<unsigned int> indicesForHomePieces() const;

  /* const*/ PlayerNumber playerNumber{0};
  Pieces pieces = {HOME, HOME, HOME, HOME};
};
