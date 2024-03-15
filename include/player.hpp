#pragma once

#include "table.hpp"
#include <array>
#include <stdexcept>


class Player {

public:
    double punctuation() const;
    void movePiece(Position pieceToMove, unsigned int positionsToMove);

    struct PieceNotFound : public std::invalid_argument {
        using std::invalid_argument::invalid_argument;
    };
    struct WrongMove : public std::invalid_argument {
        using std::invalid_argument::invalid_argument;
    };

    const PlayerNumber playerNumber;
    std::array<Position, 4> pieces = {HOME, HOME, HOME, HOME};
};