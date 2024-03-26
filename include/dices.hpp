#pragma once

#include <array>
#include <map>
#include <stdexcept>
#include <utility>

using DiceRoll = unsigned int;
using DicePairRoll = std::pair<DiceRoll, DiceRoll>;

static constexpr unsigned int DICE_FACES = 6;
static constexpr unsigned int N_DICE_ROLLS = 36;
static constexpr unsigned int N_DICE_SUM_VALUES = 12;

static constexpr std::array<DicePairRoll, N_DICE_ROLLS> loopDiceRolls() {
  std::array<DicePairRoll, N_DICE_ROLLS> dicePairs{};
  auto itDiceRoll = dicePairs.begin();
  for (unsigned int dice1 = 1; dice1 <= 6; dice1++) {
    for (unsigned int dice2 = 1; dice2 <= 6; dice2++) {
      *itDiceRoll = {dice1, dice2};
      itDiceRoll++;
    }
  }

  return dicePairs;
}

static constexpr double askAverageDiceRoll() {
  unsigned int roll{0};
  for (auto [dice1, dice2] : loopDiceRolls()) {
    roll += (dice1 + dice2);
  }

  return static_cast<double>(roll) / N_DICE_ROLLS;
}

static constexpr double averageDiceRoll = askAverageDiceRoll();

static constexpr std::array<double, 12> loadDiceValuesProbability() {
  // Count how many times we get each value from 1 to 12
  std::array<unsigned int, 12> timesSeen{};
  for (auto [dice1, dice2] : loopDiceRolls()) {
    for (DiceRoll value = 1; value <= 12; value++) {
      bool valueInDice{value == dice1 || value == dice2};
      bool valueInSum{value == dice1 + dice2};
      bool valueSeen{valueInSum || valueInDice};

      if (valueSeen) {
        timesSeen[value - 1] += 1;
      }
    }
  }

  // Convert the counter to a probability by dividing by the different dice
  // combinations we can get
  std::array<double, 12> diceValueProbability{};
  for (size_t i = 0; i < 12; i++) {
    diceValueProbability[i] =
        static_cast<double>(timesSeen[i]) / static_cast<double>(N_DICE_ROLLS);
  }

  return diceValueProbability;
}

static constexpr double getDiceValProbability(unsigned int diceVal) {
  constexpr std::array<double, 12> diceValProbability{
      loadDiceValuesProbability()};

  // Check I got a number from 1 to 12
  const unsigned int pIndex{diceVal - 1};
  if (pIndex >= 12) throw std::invalid_argument("Impossible dice value");
  return diceValProbability[pIndex];
}

// Number of dice rolls where both dices have the same value
static constexpr unsigned int N_DOUBLE_DICE_ROLLS = 6;

// Number of dice rolls where both dices have different value
static constexpr unsigned int N_NOT_DOUBLE_DICE_ROLLS =
    N_DICE_ROLLS - N_DOUBLE_DICE_ROLLS;
// Probability of every unordered not double dice roll
static constexpr double PROB_NOT_DOUBLE_DICE =
    2.0 / static_cast<double>(N_DICE_ROLLS);
// Probability of not getting double dices
static constexpr double PROB_GET_NOT_DOUBLE_DICE =
    static_cast<double>(N_NOT_DOUBLE_DICE_ROLLS) /
    static_cast<double>(N_DICE_ROLLS);

// Number of unordered dice rolls
static constexpr unsigned int N_UNIQUE_DICE_ROLLS =
    N_NOT_DOUBLE_DICE_ROLLS / 2 + N_DOUBLE_DICE_ROLLS;
// Probability of every double dice roll
static constexpr double PROB_DOUBLE_DICE =
    1.0 / static_cast<double>(N_DICE_ROLLS);
// Probability of double dices
static constexpr double PROB_GET_DOUBLE_DICE =
    static_cast<double>(N_DOUBLE_DICE_ROLLS) /
    static_cast<double>(N_DICE_ROLLS);

using UnorderedRolls = std::array<DicePairRoll, N_UNIQUE_DICE_ROLLS>;

// Return all the possible different dice rolls without repetition in the order
// of the dices
static constexpr UnorderedRolls getUnorderedRolls() {
  UnorderedRolls unorderedRolls{};

  auto unorderedRollsIt{unorderedRolls.begin()};
  for (DiceRoll dice1 = 1; dice1 <= 6; dice1++) {
    for (DiceRoll dice2 = dice1; dice2 <= 6; dice2++) {
      *unorderedRollsIt = DicePairRoll({dice1, dice2});
      unorderedRollsIt++;
    }
  }

  return unorderedRolls;
}

static constexpr double getRollProbability(DicePairRoll diceRoll) {
  return diceRoll.first == diceRoll.second ? PROB_DOUBLE_DICE
                                           : PROB_NOT_DOUBLE_DICE;
}

// Number needed to get of from home
static constexpr DiceRoll OUT_OF_HOME = 5;
