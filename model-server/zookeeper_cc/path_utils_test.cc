#include "path_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using zookeeper_cc::EnsureTrailingSlash;
using zookeeper_cc::GetLastPathSegment;
using ::testing::UnorderedElementsAre;
using ::testing::StrEq;

TEST(EnsureTrailingSlashTest, EmptyString) {
  EXPECT_EQ("/", EnsureTrailingSlash(""));
}

TEST(EnsureTrailingSlashTest, SingleSlash) {
  EXPECT_EQ("/", EnsureTrailingSlash("/"));
}

TEST(EnsureTrailingSlashTest, SeveralNodesNoTrailingSlash) {
  EXPECT_EQ("/node1/node2/", EnsureTrailingSlash("/node1/node2"));
}

TEST(EnsureTrailingSlashTest, SeveralNodesWithTrailingSlash) {
  EXPECT_EQ("/node1/node2/", EnsureTrailingSlash("/node1/node2/"));
}

TEST(GetLastPathSegmentTest, EmptyString) {
  EXPECT_THAT(GetLastPathSegment(""), StrEq(""));
}

TEST(GetLastPathSegmentTest, SingleNode) {
  EXPECT_THAT(GetLastPathSegment("node"), StrEq("node"));
}

TEST(GetLastPathSegmentTest, SingleNodeStartsWithSlash) {
  EXPECT_THAT(GetLastPathSegment("/node"), StrEq("node"));
}

TEST(GetLastPathSegmentTest, MultipleNodes) {
  EXPECT_THAT(GetLastPathSegment("/node1/node2"), StrEq("node2"));
}
