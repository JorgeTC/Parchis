#pragma once

#include <array>      // for array
#include <set>        // for set
#include <stdexcept>  // for invalid_argument
#include <string>     // for string
#include <vector>     // for vector

#include "dices.hpp"   // for DicePairRoll, DiceRoll
#include "player.hpp"  // for Player
#include "table.hpp"   // for Position, PlayerNumber

// Each time a player moves a piece
struct Move {
  PlayerNumber player;
  Position origin;
  Position dest;
};

// All the movements that occur during a player's turn
using Play = std::vector<Move>;

// The score of the table after executing the play
struct ScoredPlay {
  Play play;
  double score;
};

using MovementsSequence = std::vector<unsigned int>;

class Game {
 public:
  using Players = std::array<Player, 2>;
  using LastTouched = std::array<Position, 2>;

  // The movements a player does to get to a particular table state
  struct Turn {
    struct FinalState {
      // Final state of the game
      Players players;
      LastTouched lastTouched;
    } finalState;

    // Movements to get that state
    Play movements;
  };

  Game();
  Game(const Players&);
  Game(const Turn::FinalState&);

  std::vector<Turn> allPossibleStates(const Player&, const DicePairRoll&,
                                      unsigned int rollsInARow = 1) const;
  std::vector<Turn> tripleDouble(PlayerNumber) const;

  std::vector<Turn> allPossibleStatesFromSequence(
      const Player& currentPlayer, const MovementsSequence& advances) const;

  ScoredPlay bestPlay(PlayerNumber, DicePairRoll, unsigned int rollsInARow = 1,
                      unsigned int depth = 2) const;

  // Returns a pointer to the player who owns the piece
  // I would eat on the given position
  Player* eatenPlayer(const Player& eater, Position destPosition);

  // Move the piece and update the barrier set
  Position movePiece(PlayerNumber playerNumber, Position piece,
                     unsigned int advance);
  Position movePiece(Player& player, Position piece, unsigned int advance);

  // Take piece to position and update the barrier set
  void takePiece(PlayerNumber playerNumber, Position piece, Position dest);
  void takePiece(Player& player, Position piece, Position dest);

  // Move the piece to home and update the barrier set
  void pieceEaten(PlayerNumber playerNumber, Position eatenPiece);
  void pieceEaten(Player& player, Position eatenPiece);

  const Player& getPlayer(PlayerNumber) const;
  Player& getPlayer(PlayerNumber);
  const Player& getNextPlayer(PlayerNumber) const;
  Player& getNextPlayer(PlayerNumber);

  Position getLastTouched(PlayerNumber) const;
  void setLastTouched(PlayerNumber, Position);
  void setLastTouched(const Player&, Position);

  void updateInnerState(const Player&, Position);

  double evaluateState(const Player& currPlayer, const Player& nextPlayer,
                       unsigned int depth, unsigned int rollsInARow) const;
  double nonRecursiveEvaluateState(const Player&) const;

  Game stateAfterMovement(const Player& player, Position ori,
                          unsigned int positionsToMove) const;

  Turn::FinalState getState() const { return {players, lastTouched}; };

  struct ImpossibleMovement : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
  };

 private:
  LastTouched initLastTouched() const {
    LastTouched values{};

    constexpr auto nPlayers{std::tuple_size<decltype(players)>()};
    static_assert(nPlayers == std::tuple_size<decltype(values)>());

    for (unsigned int i = 0; i < nPlayers; i++) {
      const Player& player = players[i];

      // If no other information is given,
      // I pick the last touched piece is the first one
      values[i] = player.pieces.front();
    }

    return values;
  }

 public:
  Players players;
  std::set<Position> barriers;
  // Each player has an iterator of the container of their pieces
  LastTouched lastTouched{initLastTouched()};
};
