#include "game.hpp"

#include <array>      // for array
#include <cmath>      // for INFINITY
#include <iterator>   // for move_iterator, make_move_iterator, next
#include <sstream>    // for operator<<, ostringstream, basic_ostream, basi...
#include <stdexcept>  // for invalid_argument, logic_error

#include "player.hpp"  // for Player, Player::WrongMove
#include "table.hpp"   // for HOME, Position, isCommonPosition, PlayerNumber

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

static std::vector<Game::Turn> ulteriorMovements(
    const Player& playerToMove, const std::vector<unsigned int>& advances,
    const Game& game) {
  // If there are no more advances, return empty vector
  auto nextAdvance = std::next(advances.begin());
  if (nextAdvance == advances.end()) return {};

  return game.allPossibleStatesFromSequence(playerToMove,
                                            {nextAdvance, advances.end()});
}

std::vector<Game::Turn> Game::allPossibleStatesFromSequence(
    const Player& currentPlayer,
    const std::vector<unsigned int>& advances) const {
  // Returns all the states I can access with this sequence of movements
  // The order of the sequence is fixed

  std::vector<Game::Turn> states;

  unsigned int advance = advances.front();
  for (auto piece : currentPlayer.pieces) {
    // Create a new game to not modify the current one
    Game newGame = *this;
    // Make the current player to move the current amount
    Player& playerToMove = newGame.getPlayer(currentPlayer.playerNumber);
    Move move{currentPlayer.playerNumber, piece};
    try {
      move.dest = playerToMove.movePiece(piece, advance);
    } catch (Player::WrongMove e) {
      // The current piece cannot be moved as much as wanted, so no new state
      // can be created
      continue;
    }

    // The movement can be performed on the current piece
    // Ask for all the movements that I can do now
    std::vector<Game::Turn> nextStates =
        ulteriorMovements(playerToMove, advances, newGame);

    if (!nextStates.empty()) {
      for (auto nextState : nextStates) {
        const auto& nextMovements = nextState.movements;
        Game::Turn turn{newGame.players, {move}};
        turn.movements.insert(turn.movements.end(),
                              std::make_move_iterator(nextMovements.begin()),
                              std::make_move_iterator(nextMovements.end()));
        states.push_back(turn);
      }
    } else {
      // There are no more pieces to move, so I add to the vector the current
      // movement
      Game::Turn turn{newGame.players, {move}};
      states.push_back(turn);
    }
  }

  return states;
}

std::vector<Game::Turn> Game::allPossibleStates(
    const Player& currentPlayer, const DicePairRoll& dices) const {
  // Before the player can move must take out of home the pieces
  auto homePieces = currentPlayer.indicesForHomePieces();
  if (!homePieces.empty()) {
  }

  auto states1 =
      allPossibleStatesFromSequence(currentPlayer, {dices.first, dices.second});
  auto states2 =
      allPossibleStatesFromSequence(currentPlayer, {dices.second, dices.first});
  std::vector<Game::Turn> states;
  states.insert(states.end(), states1.begin(), states1.end());
  states.insert(states.end(), states2.begin(), states2.end());

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