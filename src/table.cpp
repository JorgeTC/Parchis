#include "table.hpp"

#include <algorithm>
#include <array>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

Position getPlayerInitialPosition(PlayerNumber player) {
  switch (player) {
    case 1:
      return 1;
    case 2:
      return 35;
    default:
      throw std::invalid_argument("Got a non existing player");
      break;
  }
}

Position getPlayerLastPosition(PlayerNumber player) {
  switch (player) {
    case 1:
      return 64;
    case 2:
      return 30;
    default:
      throw std::invalid_argument("Got a non existing player");
      break;
  }
}

bool isSafePosition(Position position) {
  const static std::set<Position> safePositions = {1,  8,  13, 18, 25, 30,
                                                   35, 42, 47, 52, 59, 64};

  return safePositions.find(position) != safePositions.end();
}

Position correctPosition(Position position) {
  // If the position is in a common position and the number is bigger than it
  // should, take it back to the range [1, totalPositions].
  // This correction is not correct if the piece is on the hallway or on the
  // goal.
  if (position > totalPositions && position < firstHallway) {
    return position - totalPositions;
  }

  return position;
}

unsigned int distanceToPosition(Position ori, Position dest) {
  if (!isCommonPosition(ori) || !isCommonPosition(dest)) {
    std::ostringstream oss;
    oss << (!isCommonPosition(ori) ? ori : dest)
        << " is not a common position.";
    throw std::invalid_argument(oss.str());
  }

  if (dest >= ori)
    return dest - ori;
  else
    return dest + totalPositions - ori;
}
