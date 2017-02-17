#include "zookeeper_cc_util/zookeeper_cc_util.h"

#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using zookeeper_cc_util::Zookeeper;
using zookeeper_cc_util::EnsureTrailingSlash;
using zookeeper_cc_util::GetLastPathSegment;
using ::testing::UnorderedElementsAre;
using ::testing::StrEq;

namespace {

const int kRecvTimeoutMs = 1000;

}

TEST(ZookeeperCcUtilEnsureTrailingSlashTest, EmptyString) {
  EXPECT_EQ("/", EnsureTrailingSlash(""));
}

TEST(ZookeeperCcUtilEnsureTrailingSlashTest, SingleSlash) {
  EXPECT_EQ("/", EnsureTrailingSlash("/"));
}

TEST(ZookeeperCcUtilEnsureTrailingSlashTest, SeveralNodesNoTrailingSlash) {
  EXPECT_EQ("/node1/node2/", EnsureTrailingSlash("/node1/node2"));
}

TEST(ZookeeperCcUtilEnsureTrailingSlashTest, SeveralNodesWithTrailingSlash) {
  EXPECT_EQ("/node1/node2/", EnsureTrailingSlash("/node1/node2/"));
}

TEST(ZookeeperCcUtilGetLastPathSegmentTest, EmptyString) {
  EXPECT_THAT(GetLastPathSegment(""), StrEq(""));
}

TEST(ZookeeperCcUtilGetLastPathSegmentTest, SingleNode) {
  EXPECT_THAT(GetLastPathSegment("node"), StrEq("node"));
}

TEST(ZookeeperCcUtilGetLastPathSegmentTest, SingleNodeStartsWithSlash) {
  EXPECT_THAT(GetLastPathSegment("/node"), StrEq("node"));
}

TEST(ZookeeperCcUtilGetLastPathSegmentTest, MultipleNodes) {
  EXPECT_THAT(GetLastPathSegment("/node1/node2"), StrEq("node2"));
}

class ZookeeperCcUtilTest : public ::testing::Test {
 protected:
  virtual void SetUp() override {
    char *hosts = getenv("ZOOKEEPER_TEST_HOSTS");
    ASSERT_TRUE(hosts && strlen(hosts) > 0) << "ZOOKEEPER_TEST_HOSTS is unspecified";

    // Construct random path inside Zookeeper to provide some tests isolation.
    auto unit_test = ::testing::UnitTest::GetInstance();
    const ::testing::TestInfo* const test_info =
        unit_test->current_test_info();
    std::stringstream bases;
    bases << "/zookeeper-cc-util-test"
          << "/" << unit_test->random_seed()
          << "/" << test_info->test_case_name()
          << "/" << test_info->name();
    std::string base = bases.str();

    // Disable most logging.
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);

    zk_.reset(new Zookeeper(hosts, kRecvTimeoutMs, base));
  }

  virtual void TearDown() override {
    zk_.reset();
  }

  std::unique_ptr<Zookeeper> zk_;
};

TEST_F(ZookeeperCcUtilTest, EnforceBasePathAndExists) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("", &ZOO_OPEN_ACL_UNSAFE));

  EXPECT_EQ(ZOK, zk_->Exists("", nullptr, nullptr));
}

TEST_F(ZookeeperCcUtilTest, EnforceNodeAndExists) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("some_node", &ZOO_OPEN_ACL_UNSAFE));

  EXPECT_EQ(ZOK, zk_->Exists("some_node", nullptr, nullptr));
}

TEST_F(ZookeeperCcUtilTest, NodeNotExists) {
  ASSERT_TRUE(zk_->Init());

  EXPECT_EQ(ZNONODE, zk_->Exists("some_node", nullptr, nullptr));
}

TEST_F(ZookeeperCcUtilTest, GetCreateGet) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("", &ZOO_OPEN_ACL_UNSAFE));

  std::string data = "not_modified";

  EXPECT_EQ(ZNONODE, zk_->Get("some_node", &data, nullptr, nullptr));
  EXPECT_EQ("not_modified", data);

  EXPECT_EQ(ZOK, zk_->Create("some_node", "some_value", false, &ZOO_OPEN_ACL_UNSAFE));

  EXPECT_EQ(ZOK, zk_->Get("some_node", &data, nullptr, nullptr));
  EXPECT_EQ("some_value", data);
}

TEST_F(ZookeeperCcUtilTest, SetGet) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("some_node", &ZOO_OPEN_ACL_UNSAFE));

  EXPECT_EQ(ZOK, zk_->Set("some_node", "some_value"));

  std::string data;
  EXPECT_EQ(ZOK, zk_->Get("some_node", &data, nullptr, nullptr));
  EXPECT_EQ("some_value", data);
}

TEST_F(ZookeeperCcUtilTest, GetChildren) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("node1/1", &ZOO_OPEN_ACL_UNSAFE));
  ASSERT_EQ(ZOK, zk_->EnforcePath("node1/2", &ZOO_OPEN_ACL_UNSAFE));
  ASSERT_EQ(ZOK, zk_->EnforcePath("node2/3", &ZOO_OPEN_ACL_UNSAFE));
  ASSERT_EQ(ZOK, zk_->EnforcePath("node2/4", &ZOO_OPEN_ACL_UNSAFE));

  std::vector<std::string> children;
  EXPECT_EQ(ZOK, zk_->GetChildren("node1", &children, nullptr));
  EXPECT_THAT(children, UnorderedElementsAre("1", "2"));

  EXPECT_EQ(ZOK, zk_->GetChildren("node2", &children, nullptr));
  EXPECT_THAT(children, UnorderedElementsAre("3", "4"));

  EXPECT_EQ(ZOK, zk_->GetChildren("", &children, nullptr));
  EXPECT_THAT(children, UnorderedElementsAre("node1", "node2"));
}
