#pragma once

using Position = unsigned int;

// Total amount of common positions where the pieces can be.
// There is no position 0, which means totalPositions is a valid position
static constexpr unsigned int totalPositions = 68;

// Number of coloured positions in the hallway before getting to the goal.
// It does not count the goal itself.
static constexpr unsigned int hallwayLength = 7;

// First position in the final hallway.
// Using 101 instead of 100 to follow the game convention of numbering positions
// from 1 instead of from 0.
static constexpr Position firstHallway = 101;

// Goal position
static constexpr Position GOAL = firstHallway + hallwayLength;

// Last position in the hallway
static constexpr Position finalHallway = GOAL - 1;

// Home position
static constexpr Position HOME = 0;

// Number to identify a player
using PlayerNumber = unsigned int;

// Returns the position where the player should move its pieces when it starts
// playing
Position getPlayerInitialPosition(PlayerNumber player);

// Returns the position just before eneterig the last hallway to goal
Position getPlayerLastPosition(PlayerNumber player);

// Returns whether a piece in this position can be eaten
bool isSafePosition(Position position);

// Returns whether a position is a common one, nor home, hallway or goal
static constexpr bool isCommonPosition(Position position) {
  return (position >= 1 && position <= totalPositions);
};

// Returns whether a position is in the hallway, it does not include the goal
static constexpr bool isHallwayPosition(Position position) {
  return position >= firstHallway && position <= finalHallway;
};

// If the number is too big, take it back to the correct range
Position correctPosition(Position position);

// Returns the distance to get from one common position to other
unsigned int distanceToPosition(Position ori, Position dest);