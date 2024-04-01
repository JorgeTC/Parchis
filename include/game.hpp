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

  // The movements a player does to get to a particular table state
  struct Turn {
    Players finalSate;
    Play movements;
  };

  Game();
  Game(const Players&);

  std::vector<Turn> allPossibleStates(const Player&, const DicePairRoll&) const;
  std::vector<Turn> allPossibleStatesFromSequence(
      const Player& currentPlayer,
      const MovementsSequence& advances) const;

  ScoredPlay bestPlay(PlayerNumber, DicePairRoll);
  ScoredPlay bestPlay(const Player&, DicePairRoll);
  ScoredPlay bestPlay(const Player&, DiceRoll);

  Player* eatenPlayer(const Player& eater, Position destPosition);

  const Player& getPlayer(PlayerNumber) const;
  Player& getPlayer(PlayerNumber);

  double evaluateState(const Player&, unsigned int depth) const;
  double nonRecursiveEvaluateState(const Player&) const;

  Game stateAfterMovement(const Player& player, Position ori,
                          unsigned int positionsToMove) const;

  struct ImpossibleMovement : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
  };

  Players players;
  std::set<Position> bridges;
};
