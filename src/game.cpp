#include "game.hpp"

#include <algorithm>  // for find, remove_if
#include <array>      // for array
#include <cmath>      // for INFINITY
#include <iterator>   // for move_iterator, next, make_move_iterator
#include <set>        // for set, operator==, erase_if, set<>::const_iterator
#include <sstream>    // for operator<<, ostringstream, basic_ostream, basi...
#include <stdexcept>  // for invalid_argument, logic_error

#include "player.hpp"  // for Player, Player::WrongMove, Player::PieceNotFound
#include "table.hpp"   // for HOME, Position, PlayerNumber, getPlayerInitial...

static constexpr unsigned int EXTRA_MOVEMENT_ON_GOAL = 10;
static constexpr unsigned int EXTRA_MOVEMENT_ON_KILL = 20;

static constexpr Game::Players loadPlayers() {
  return {Player({1, {HOME, HOME, HOME, HOME}}),
          Player({2, {HOME, HOME, HOME, HOME}})};
}

static std::set<Position> loadBarriers(const Game::Players& players) {
  std::set<Position> notEmptyPositions{};
  std::set<Position> barriers{};

  for (const Player& player : players) {
    for (const Position piece : player.pieces) {
      // If the position cannot have a barrier, go on
      if (!isCommonPosition(piece)) continue;

      // Check I know there is another piece on this position
      if (notEmptyPositions.find(piece) != notEmptyPositions.end()) {
        // then I know this second piece creates a barrier.
        barriers.insert(piece);
      } else {
        // It is the first piece I see on this position,
        // so I store it in case I see another piece in this position
        notEmptyPositions.insert(piece);
      }
    }
  }

  return barriers;
}

Game::Game() : players(loadPlayers()), barriers(loadBarriers(players)) {}

Game::Game(const Players& players)
    : players(players), barriers(loadBarriers(players)){};

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
  // Check I have pieces to take out from home
  auto homePieces = currentPlayer.indicesForHomePieces();
  bool hasPiecesAtHome = !homePieces.empty();
  if (!hasPiecesAtHome) return false;

  // Check on the inital position the is space for one more piece
  // Only need to check I have not two pieces of mine on the initial position
  Position initialPosition =
      getPlayerInitialPosition(currentPlayer.playerNumber);
  bool isSpaceInInitialPosition =
      currentPlayer.countPiecesInPosition(initialPosition) < 2;

  return isSpaceInInitialPosition;
}

static std::vector<MovementsSequence> movementsSequences(
    const Player& currentPlayer, const DicePairRoll& dices) {
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

static Player* eatenPlayerOnSafePosition(const Player& eater,
                                         Game::Players& players,
                                         Position destPosition) {
  Player* eaten{nullptr};
  unsigned int piecesCounter = 0;
  // I must check there are three pieces on this position and return the enemy
  // who is here
  for (Player& player : players) {
    // If any of the pieces of this player is in the same position,
    // I have eaten it
    for (Position piece : player.pieces) {
      if (piece != destPosition) continue;

      // Count a piece that is on this position
      piecesCounter += 1;
      // If the piece is from an enemy, store it
      if (player.playerNumber != eater.playerNumber) {
        eaten = &player;
      }
    }
  }

  // If there are three pieces, there is no space
  // for the one that have just arrived
  // so one of the pieces that were there will be eaten.
  if (piecesCounter == 3) {
    return eaten;
  }
  // There is space for the piece so no other piece is sent to home
  else {
    return nullptr;
  }
}

Player* Game::eatenPlayer(const Player& eater, Position destPosition) {
  if (!isEatingPosition(destPosition)) {
    // If the position is not dangerous, no further considerations
    if (destPosition != getPlayerInitialPosition(eater.playerNumber)) {
      return nullptr;
    }

    // It is possible to eat the adversary if there are three pieces on this
    // position. One of those will be from the enemy
    if (barriers.find(destPosition) != barriers.end()) {
      return eatenPlayerOnSafePosition(eater, players, destPosition);
    } else {
      return nullptr;
    }
  }

  // Search for a player with a piece in the position
  for (Player& player : players) {
    // I cannot eat myself
    if (player.playerNumber == eater.playerNumber) continue;

    // If any of the pieces of this player is in the same position,
    // I have eaten it
    for (Position piece : player.pieces) {
      if (piece == destPosition) return &player;
    }
  }

  return nullptr;
}

Position Game::movePiece(PlayerNumber playerNumber, Position piece,
                         unsigned int advance) {
  Player& playerToMove = getPlayer(playerNumber);
  return movePiece(playerToMove, piece, advance);
};

Position Game::movePiece(Player& player, Position piece, unsigned int advance) {
  Position destPosition = player.movePiece(piece, advance, barriers);
  updateInnerState(player, destPosition);

  return destPosition;
};

void Game::takePiece(PlayerNumber playerNumber, Position piece, Position dest) {
  Player& playerToMove = getPlayer(playerNumber);
  takePiece(playerToMove, piece, dest);
};

void Game::takePiece(Player& player, Position piece, Position dest) {
  auto itPiece = std::find(player.pieces.begin(), player.pieces.end(), piece);
  if (itPiece == player.pieces.end()) {
    throw Player::PieceNotFound("No piece to be moved");
  }

  Position& pieceToMove = *itPiece;
  pieceToMove = dest;
  updateInnerState(player, dest);
};

void Game::pieceEaten(PlayerNumber playerNumber, Position eatenPiece) {
  Player& playerToMove = getPlayer(playerNumber);
  pieceEaten(playerToMove, eatenPiece);
};

void Game::pieceEaten(Player& player, Position eatenPiece) {
  player.pieceEaten(eatenPiece);

  updateInnerState(player, HOME);
};

void Game::updateInnerState(const Player& player, Position destPosition) {
  barriers = loadBarriers(players);
  setLastTouched(player, destPosition);
}

static bool doubleDices(const MovementsSequence& advances) {
  return advances.size() == 2 && advances.front() == advances.back();
}

static bool doubleDices(const DicePairRoll& dices) {
  return dices.first == dices.second;
}

static std::set<Position> piecesOnBarrier(const Player& currentPlayer,
                                          const std::set<Position>& barriers) {
  std::set<Position> uniquePiecesOnBarrier;
  for (Position piece : currentPlayer.pieces) {
    bool isPieceOnABarrier = barriers.find(piece) != barriers.end();
    if (isPieceOnABarrier) {
      uniquePiecesOnBarrier.insert(piece);
    }
  }

  return uniquePiecesOnBarrier;
}

static bool pieceCanBeMoved(Position piece, PlayerNumber playerNumber,
                            unsigned int advance, Game currentGame) {
  Player& playerToMove = currentGame.getPlayer(playerNumber);
  try {
    // Try to move the piece calling the Player object because its function is a
    // bit lighter
    playerToMove.movePiece(piece, advance);
  } catch (Player::WrongMove e) {
    // The current piece cannot be moved as much as wanted
    return false;
  }

  // The piece was moved successfully
  return true;
}

static void filterPiecesThatCanBeMoved(std::set<Position>& pieces,
                                       PlayerNumber playerNumber,
                                       unsigned int advance,
                                       const Game& currentGame) {
  std::erase_if(pieces, [&](Position piece) {
    return !pieceCanBeMoved(piece, playerNumber, advance, currentGame);
  });
}

std::vector<Game::Turn> Game::allPossibleStatesFromSequence(
    const Player& currentPlayer, const MovementsSequence& advances) const {
  // Returns all the states I can access with this sequence of movements
  // The order of the sequence is fixed

  std::vector<Game::Turn> states;

  // Take the advance I will try to perform
  unsigned int advance = advances.front();

  std::set<Position> piecesToMove;

  // If the advance is 5 and I have pieces to take out from home, I cannot move
  // any other piece
  if (advance == OUT_OF_HOME && canTakeOutPieces(currentPlayer)) {
    piecesToMove = {HOME};
  }
  // Check if I got doubles dices
  else if (doubleDices(advances)) {
    // Ask all the pieces that are on a barrier
    std::set<Position> barrierPieces = piecesOnBarrier(currentPlayer, barriers);
    // all of them are candidates to be moved.
    piecesToMove = barrierPieces;
    // Remove the barriers that cannot be broken
    filterPiecesThatCanBeMoved(piecesToMove, currentPlayer.playerNumber,
                               advance, *this);

    // There is no barrier that can be broken,
    // fill piecesToMove with the pieces that are not in barriers
    if (piecesToMove.empty()) {
      for (Position piece : currentPlayer.pieces) {
        if (barrierPieces.find(piece) != barrierPieces.end())
          piecesToMove.insert(piece);
      }
    }
  }

  // If I have not mandatory pieces to move, all the pieces are candidates to be
  // moved
  if (piecesToMove.empty()) {
    const auto& playerPieces = currentPlayer.pieces;
    piecesToMove.insert(playerPieces.begin(), playerPieces.end());
  }

  for (Position piece : piecesToMove) {
    // Create a new game to not modify the current one
    Game newGame = *this;
    // Make the current player to move the current amount
    Player& playerToMove = newGame.getPlayer(currentPlayer.playerNumber);
    Move move{currentPlayer.playerNumber, piece};
    try {
      // Move the piece calling the Game object to get the barriers updated
      move.dest = newGame.movePiece(playerToMove, piece, advance);
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
      newGame.pieceEaten(*eatenPlayer, move.dest);
      // Store the movement
      Move killingMove{eatenPlayer->playerNumber, move.dest, HOME};
      decisionMovements.push_back(killingMove);
    }

    // The movement can be performed on the current piece
    // Ask for all the movements that I can do now
    std::vector<Turn> nextStates = ulteriorMovements(
        playerToMove, advances, newGame, gotToGoal, haveEaten);

    if (!nextStates.empty()) {
      for (const Turn& nextState : nextStates) {
        const Play& nextMovements = nextState.movements;
        Turn turn{nextState.finalSate, decisionMovements};
        turn.movements.insert(turn.movements.end(),
                              std::make_move_iterator(nextMovements.begin()),
                              std::make_move_iterator(nextMovements.end()));
        states.push_back(turn);
      }
    } else {
      // There are no more pieces to move,
      // so I add to the vector the current movement
      Turn turn{newGame.players, decisionMovements};
      states.push_back(turn);
    }
  }

  return states;
}

bool operator==(const Move& m1, const Move& m2) {
  return m1.player == m2.player && m1.origin == m2.origin && m1.dest == m2.dest;
}

static bool hasMovedABarrier(const Game& currentGame, const Game::Turn& turn) {
  // If I got a double dice I must break a barrier.
  // This means that if I moved one element of the barrier,
  // the other one cannot move to make another barrier just after the recent
  // broken one.

  const std::set<Position>& barriers = currentGame.barriers;
  // Check the first piece I moved is in a barrier
  const Play& movements = turn.movements;
  const Move& firstMove = movements.front();
  Position firstMovedPiece = firstMove.origin;
  bool brokeBarrier{barriers.find(firstMovedPiece) != barriers.end()};
  if (!brokeBarrier) return false;

  for (auto itMovement = std::next(movements.begin());
       itMovement != movements.end(); itMovement++) {
    const Move& movement = *itMovement;
    if (movement == firstMove) {
      return true;
    }
  }

  return false;
}

std::vector<Game::Turn> Game::tripleDouble(PlayerNumber playerNumber) const {
  Position lastTouchedPosition = *getLastTouched(playerNumber);

  // If the last touched piece can go back to HOME
  if (isCommonPosition(lastTouchedPosition)) {
    Game newGame = *this;
    newGame.pieceEaten(playerNumber, lastTouchedPosition);
    Move goHomeMove{playerNumber, lastTouchedPosition, HOME};
    return {{newGame.players, Play({goHomeMove})}};
  } else {
    // If the piece cannot go back home I cannot make movement anyway
    // So leave the table as it is and go to the next player
    return {{players, Play({})}};
  }
};

std::vector<Game::Turn> Game::allPossibleStates(
    const Player& currentPlayer, const DicePairRoll& dices,
    unsigned int rollsInARow /* = 1*/) const {
  // If this is the third double, exit the function and take the last touched
  // piece to HOME
  if (rollsInARow == 3 && doubleDices(dices)) {
    return tripleDouble(currentPlayer.playerNumber);
  }

  // From de dices get the sequences of movements
  auto possibleMovements = movementsSequences(currentPlayer, dices);
  std::vector<Turn> states;
  for (const auto& sequence : possibleMovements) {
    std::vector<Turn> statesForSequence =
        allPossibleStatesFromSequence(currentPlayer, sequence);
    states.insert(states.end(), statesForSequence.begin(),
                  statesForSequence.end());
  }

  // If I got double dices, reject the combinations
  // of movements that have moved a barrier.
  // Barriers must be broken, not moved
  if (dices.first == dices.second) {
    std::vector<Turn> filteredStates = states;
    auto filter = std::remove_if(
        filteredStates.begin(), filteredStates.end(),
        [&](const Turn& turn) { return hasMovedABarrier(*this, turn); });
    filteredStates.erase(filter, filteredStates.end());

    // If there are no movements to be done, allow moving the barrier
    if (!filteredStates.empty()) states = filteredStates;
  }

  return states;
}

double Game::nonRecursiveEvaluateState(const Player& currentPlayer) const {
  double value{0.0};
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
  std::vector<Turn> turns{allPossibleStates(player, dices)};
  for (const Turn& turn : turns) {
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
    playerToMove.movePiece(ori, positionsToMove, barriers);
  } catch (const Player::WrongMove& moveException) {
    std::ostringstream oss;
    oss << "Piece at position " << ori << " cannot be moved with a "
        << positionsToMove;

    throw ImpossibleMovement(oss.str());
  }

  return Game(copiedPlayers);
}

Player::Pieces::const_iterator Game::getLastTouched(
    PlayerNumber playerNumber) const {
  constexpr std::size_t nPlayers{std::tuple_size<decltype(players)>()};
  if (playerNumber >= nPlayers) {
    throw std::invalid_argument("Got a non existing player");
  }

  return lastTouched[playerNumber - 1];
};

void Game::setLastTouched(PlayerNumber playerNumber,
                          Position lastTouchedPosition) {
  Player& playerToUpdate = getPlayer(playerNumber);
  setLastTouched(playerToUpdate, lastTouchedPosition);
};

void Game::setLastTouched(const Player& player, Position lastTouchedPosition) {
  const Player::Pieces& pieces = player.pieces;
  auto itLastTouched =
      std::find(pieces.begin(), pieces.end(), lastTouchedPosition);
  if (itLastTouched == pieces.end()) {
    throw Player::PieceNotFound("Wrong piece as last moved");
  }

  lastTouched[player.playerNumber - 1] = itLastTouched;
};