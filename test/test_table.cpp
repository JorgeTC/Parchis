#include <gtest/gtest.h>  // for Test, Message, TestPartResult, EXPECT_THROW

#include <memory>     // for allocator
#include <stdexcept>  // for invalid_argument

#include "table.hpp"  // for distanceToPosition, getPlayerInitialPosition

TEST(TableTest, ErrorOnWrongPlayer) {
  EXPECT_THROW(getPlayerInitialPosition(0), std::invalid_argument);
}

TEST(TableTest, Distance) {
  ASSERT_EQ(distanceToPosition(1, 1), 0);
  ASSERT_EQ(distanceToPosition(1, 25), 24);
  ASSERT_EQ(distanceToPosition(68, 1), 1);

  EXPECT_THROW(distanceToPosition(HOME, 1), std::invalid_argument);
  EXPECT_THROW(distanceToPosition(GOAL, 1), std::invalid_argument);
  EXPECT_THROW(distanceToPosition(firstHallway, finalHallway),
               std::invalid_argument);
}