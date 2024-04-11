#include "table.hpp"

#include <sstream>    // for operator<<, ostringstream, basic_ostream, basic...
#include <stdexcept>  // for invalid_argument

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
