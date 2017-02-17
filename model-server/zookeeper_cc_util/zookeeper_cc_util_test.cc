#include "zookeeper_cc_util/zookeeper_cc_util.h"

#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <gtest/gtest.h>

using zookeeper_cc_util::Zookeeper;

namespace {

const int kRecvTimeoutMs = 1000;

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
