#include "game.hpp"

#include <array>      // for array
#include <cmath>      // for INFINITY
#include <iterator>   // for move_iterator, make_move_iterator, next
#include <sstream>    // for operator<<, ostringstream, basic_ostream, basi...
#include <stdexcept>  // for invalid_argument, logic_error

#include "player.hpp"  // for Player, Player::WrongMove
#include "table.hpp"   // for HOME, Position, isCommonPosition, PlayerNumber

static constexpr unsigned int EXTRA_MOVEMENT_ON_GOAL = 10;
static constexpr unsigned int EXTRA_MOVEMENT_ON_KILL = 20;

static constexpr Game::Players loadPlayers() {
  return {Player({1, {HOME, HOME, HOME, HOME}}),
          Player({2, {HOME, HOME, HOME, HOME}})};
}

static std::set<Position> loadBridges(const Game::Players& players) {
  std::set<Position> bridges{};
  for (Position piece1 : players[0].pieces) {
    if (!isCommonPosition(piece1)) continue;
    for (Position piece2 : players[1].pieces) {
      if (!isCommonPosition(piece2)) continue;
      if (piece1 == piece2) bridges.insert(piece1);
    }
  }

  return bridges;
}

Game::Game() : players(loadPlayers()), bridges(loadBridges(players)) {}

Game::Game(const Players& players)
    : players(players), bridges(loadBridges(players)){};

Player& getPlayer(Game::Players& players, PlayerNumber player) {
  switch (player) {
    case 1:
    case 2:
      return players[player - 1];
    default:
      throw std::invalid_argument("Got a non existing player");
      break;
  }
};

const Player& getPlayer(const Game::Players& players, PlayerNumber player) {
  return const_cast<const Player&>(
      getPlayer(const_cast<Game::Players&>(players), player));
};

const Player& Game::getPlayer(PlayerNumber player) const {
  return const_cast<const Player&>(const_cast<Game*>(this)->getPlayer(player));
};

Player& Game::getPlayer(PlayerNumber player) {
  return ::getPlayer(players, player);
};

static Move constructMove(const Player& oldPlayer, const Player& newPlayer) {
  if (oldPlayer.playerNumber != newPlayer.playerNumber) {
    std::ostringstream oss;
    oss << "Old player and new player must have same id. "
        << oldPlayer.playerNumber << " and " << newPlayer.playerNumber
        << " were given instead.";

    throw std::invalid_argument(oss.str());
  }

  const auto& oldPieces = oldPlayer.pieces;
  const auto& newPieces = newPlayer.pieces;

  for (int i = 0; i < oldPieces.size(); i++) {
    if (oldPieces[i] != newPieces[i]) {
      return Move{newPlayer.playerNumber, oldPieces[i], newPieces[i]};
    }
  }

  std::ostringstream oss;
  oss << "No different pieces were found";
  throw std::invalid_argument(oss.str());
}

static std::vector<Game::Turn> ulteriorMovementsWithBoost(
    const Player& playerToMove,
    MovementsSequence::const_iterator advances_begin,
    MovementsSequence::const_iterator advances_end, const Game& game,
    unsigned int boostAdvance) {
  // Get the movements I have to do and add the boost
  MovementsSequence nextMovements;
  nextMovements.push_back(boostAdvance);
  nextMovements.insert(nextMovements.end(), advances_begin, advances_end);

  // Return the sequence of movements that can be done with this sequence of
  // advances
  auto movementsWithBoost{
      game.allPossibleStatesFromSequence(playerToMove, nextMovements)};
  return movementsWithBoost;
}

static std::vector<Game::Turn> ulteriorMovements(
    const Player& playerToMove, const MovementsSequence& advances,
    const Game& game, bool gotToGoal, bool haveEaten) {
  // Discard the already performed advance
  auto nextAdvance{std::next(advances.begin())};

  if (gotToGoal) {
    // Sequence of movements adding the boost
    auto movementsWithGoalBoost =
        ulteriorMovementsWithBoost(playerToMove, nextAdvance, advances.end(),
                                   game, EXTRA_MOVEMENT_ON_GOAL);
    // If the vector is empty, boost cannot be performed;
    // but the rest of the dices must be executed. Cannot return.
    if (!movementsWithGoalBoost.empty()) return movementsWithGoalBoost;
  }
  if (haveEaten) {
    // Sequence of movements adding the boost
    auto movementsWithKillBoost =
        ulteriorMovementsWithBoost(playerToMove, nextAdvance, advances.end(),
                                   game, EXTRA_MOVEMENT_ON_KILL);
    // If the vector is empty, boost cannot be performed;
    // but the rest of the dices must be executed. Cannot return.
    if (!movementsWithKillBoost.empty()) return movementsWithKillBoost;
  }

  // If there are no more advances, return empty vector
  if (nextAdvance == advances.end()) return {};

  return game.allPossibleStatesFromSequence(playerToMove,
                                            {nextAdvance, advances.end()});
}

static bool canTakeOutPieces(const Player& currentPlayer) {
  auto homePieces = currentPlayer.indicesForHomePieces();
  return !homePieces.empty();
}

std::vector<MovementsSequence> movementsSequences(const Player& currentPlayer,
                                                  const DicePairRoll& dices) {
  // If we can take out a piece we must move the 5 first of all
  if (canTakeOutPieces(currentPlayer)) {
    if (dices.first + dices.second == OUT_OF_HOME)
      return {{OUT_OF_HOME}};
    else if (dices.first == OUT_OF_HOME)
      return {{dices.first, dices.second}};
    else if (dices.second == OUT_OF_HOME)
      return {{dices.second, dices.first}};
  }

  std::vector<MovementsSequence> movements;

  // Regular case, no mandatory movements
  movements.push_back({dices.first, dices.second});
  if (dices.first != dices.second)
    movements.push_back({dices.second, dices.first});
  return movements;
}

Player* Game::eatenPlayer(const Player& eater, Position destPosition) {
  // If the position is not dangerous, no further considerations
  if (!isEatingPosition(destPosition)) return nullptr;

  // Search for a player with a piece in the position
  for (Player& player : players) {
    // I cannot eat myself
    if (player.playerNumber == eater.playerNumber) continue;

    // If any of the pieces of this player is in the same position,
    // I have eaten it
    for (auto piece : player.pieces) {
      if (piece == destPosition) return &player;
    }
  }

  return nullptr;
}

std::vector<Game::Turn> Game::allPossibleStatesFromSequence(
    const Player& currentPlayer, const MovementsSequence& advances) const {
  // Returns all the states I can access with this sequence of movements
  // The order of the sequence is fixed

  std::vector<Game::Turn> states;

  // Take the advance I will try to perform
  unsigned int advance = advances.front();

  std::array<Position, 4> piecesToMove{currentPlayer.pieces};
  // If the advance is 5 and I have pieces to take out from home, I cannot move
  // any other piece
  if (advance == OUT_OF_HOME) {
    if (canTakeOutPieces(currentPlayer)) {
      piecesToMove = {HOME, HOME, HOME, HOME};
    }
  }

  for (auto piece : piecesToMove) {
    // Create a new game to not modify the current one
    Game newGame = *this;
    // Make the current player to move the current amount
    Player& playerToMove = newGame.getPlayer(currentPlayer.playerNumber);
    Move move{currentPlayer.playerNumber, piece};
    try {
      move.dest = playerToMove.movePiece(piece, advance);
    } catch (Player::WrongMove e) {
      // The current piece cannot be moved as much as wanted,
      // so no new state can be created
      continue;
    }

    // Check I got to goal, which lets me advance 10 positions
    bool gotToGoal{move.dest == GOAL};

    // Movements derived from the current movement
    std::vector<Move> decisionMovements{move};

    // Check I ate one rival piece
    Player* eatenPlayer{newGame.eatenPlayer(playerToMove, move.dest)};
    bool haveEaten{eatenPlayer != nullptr};
    // I ate someone, add the movement of taking its piece back home
    if (haveEaten) {
      // Execute the movement to home
      eatenPlayer->pieceEaten(move.dest);
      // Store the movement
      Move killingMove{eatenPlayer->playerNumber, move.dest, HOME};
      decisionMovements.push_back(killingMove);
    }

    // The movement can be performed on the current piece
    // Ask for all the movements that I can do now
    std::vector<Game::Turn> nextStates = ulteriorMovements(
        playerToMove, advances, newGame, gotToGoal, haveEaten);

    if (!nextStates.empty()) {
      for (auto nextState : nextStates) {
        const auto& nextMovements = nextState.movements;
        Game::Turn turn{nextState.finalSate, decisionMovements};
        turn.movements.insert(turn.movements.end(),
                              std::make_move_iterator(nextMovements.begin()),
                              std::make_move_iterator(nextMovements.end()));
        states.push_back(turn);
      }
    } else {
      // There are no more pieces to move, so I add to the vector the current
      // movement
      Game::Turn turn{newGame.players, decisionMovements};
      states.push_back(turn);
    }
  }

  return states;
}

std::vector<Game::Turn> Game::allPossibleStates(
    const Player& currentPlayer, const DicePairRoll& dices) const {
  // From de dices get the sequences of movements
  auto possibleMovements = movementsSequences(currentPlayer, dices);
  std::vector<Game::Turn> states;
  for (const auto& sequence : possibleMovements) {
    std::vector<Game::Turn> statesForSequence =
        allPossibleStatesFromSequence(currentPlayer, sequence);
    states.insert(states.end(), statesForSequence.begin(),
                  statesForSequence.end());
  }
  return states;
}

double Game::nonRecursiveEvaluateState(const Player& currentPlayer) const {
  double value = 0;
  for (const Player& player : players) {
    double playerValue = player.punctuation();
    if (player.playerNumber == currentPlayer.playerNumber) playerValue *= -1;
    value -= playerValue;
  }

  return value;
}

double Game::evaluateState(const Player& currentPlayer,
                           unsigned int depth) const {
  if (depth == 0) {
    return nonRecursiveEvaluateState(currentPlayer);
  }
  throw std::logic_error("Not implemented error");
}

ScoredPlay Game::bestPlay(PlayerNumber playerId, DicePairRoll dices) {
  const Player& player{getPlayer(playerId)};

  ScoredPlay bestPlay = {{}, INFINITY};
  // Get all the possible states I can get with this dice roll
  std::vector<Game::Turn> turns{allPossibleStates(player, dices)};
  for (auto turn : turns) {
    // Get the final state of the player that has made a movement
    const Player& finalPlayerSate =
        ::getPlayer(turn.finalSate, player.playerNumber);

    // If I find a turn for which I win, stop searching
    if (finalPlayerSate.hasWon()) {
      return {turn.movements, finalPlayerSate.punctuation()};
    }

    // Evaluate the current state with the needed depth
    double evaluation = Game(turn.finalSate).evaluateState(finalPlayerSate, 0);
    // If the state is better that the best found till now, update the movements
    if (evaluation < bestPlay.score) {
      bestPlay = {turn.movements, evaluation};
    }
  }

  // Return the best movements
  return bestPlay;
};

Game Game::stateAfterMovement(const Player& player, Position ori,
                              unsigned int positionsToMove) const {
  Players copiedPlayers = players;
  Player& playerToMove = copiedPlayers[player.playerNumber - 1];
  try {
    playerToMove.movePiece(ori, positionsToMove);
  } catch (const Player::WrongMove& moveException) {
    std::ostringstream oss;
    oss << "Piece at position " << ori << " cannot be moved with a "
        << positionsToMove;

    throw ImpossibleMovement(oss.str());
  }

  return Game(copiedPlayers);
}
