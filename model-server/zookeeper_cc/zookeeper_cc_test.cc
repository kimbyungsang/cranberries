#include "zookeeper_cc/zookeeper_cc.h"

#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <iostream>
#include <sstream>
#include <future>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using zookeeper_cc::Zookeeper;
using ::testing::UnorderedElementsAre;
using ::testing::AnyOf;
using ::testing::Eq;

namespace {

const int kRecvTimeoutMs = 1000;
const int kConnectionRestoreRetries = 20;
const int kConnectionRestoreDelayUs = 100 * 1000;
static_assert(kRecvTimeoutMs <
    kConnectionRestoreRetries * kConnectionRestoreDelayUs,
    "Total connection restore time should be greater than recv timeout");

const auto kWatcherTimeout = std::chrono::milliseconds(100);

template<typename R>
bool is_ready(const std::future<R> &f) {
  return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template<typename R>
bool wait_ready(const std::future<R> &f) {
  return f.wait_for(kWatcherTimeout) == std::future_status::ready;
}

}

class ZookeeperCcTest : public ::testing::Test {
 protected:
  virtual void SetUp() override {
    char *hosts = getenv("ZOOKEEPER_TEST_HOSTS");
    ASSERT_TRUE(hosts && strlen(hosts) > 0) << "ZOOKEEPER_TEST_HOSTS is unspecified";

    char *container_id = getenv("ZOOKEEPER_TEST_DOCKER_CONTAINER_ID");
    ASSERT_TRUE(container_id && strlen(container_id) > 0)
        << "ZOOKEEPER_TEST_DOCKER_CONTAINER_ID is unspecified";
    container_id_ = container_id;

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

  bool StartZookeeper() {
    if (system(("docker start " + container_id_ + " >/dev/null").c_str()) != 0) {
      return false;
    }
    for (int i = 0; i < kConnectionRestoreRetries; i++) {
      if (zk_->State() == ZOO_CONNECTED_STATE) {
        return true;
      }
      usleep(kConnectionRestoreDelayUs);
    }
    return false;
  }

  bool StopZookeeper() {
    return system(("docker stop " + container_id_ + " >/dev/null").c_str()) == 0;
  }

 private:
  std::string container_id_;
};

TEST_F(ZookeeperCcTest, EnforceBasePathAndExists) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("", &ZOO_OPEN_ACL_UNSAFE));

  EXPECT_EQ(ZOK, zk_->Exists("", nullptr, nullptr));
}

TEST_F(ZookeeperCcTest, EnforceNodeAndExists) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("some_node", &ZOO_OPEN_ACL_UNSAFE));

  EXPECT_EQ(ZOK, zk_->Exists("some_node", nullptr, nullptr));
}

TEST_F(ZookeeperCcTest, NodeNotExists) {
  ASSERT_TRUE(zk_->Init());

  EXPECT_EQ(ZNONODE, zk_->Exists("some_node", nullptr, nullptr));
}

TEST_F(ZookeeperCcTest, GetCreateGet) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("", &ZOO_OPEN_ACL_UNSAFE));

  std::string data = "not_modified";

  EXPECT_EQ(ZNONODE, zk_->Get("some_node", &data, nullptr, nullptr));
  EXPECT_EQ("not_modified", data);

  EXPECT_EQ(ZOK, zk_->Create("some_node", "some_value", false, &ZOO_OPEN_ACL_UNSAFE));

  EXPECT_EQ(ZOK, zk_->Get("some_node", &data, nullptr, nullptr));
  EXPECT_EQ("some_value", data);
}

TEST_F(ZookeeperCcTest, SetGet) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("some_node", &ZOO_OPEN_ACL_UNSAFE));

  EXPECT_EQ(ZOK, zk_->Set("some_node", "some_value"));

  std::string data;
  EXPECT_EQ(ZOK, zk_->Get("some_node", &data, nullptr, nullptr));
  EXPECT_EQ("some_value", data);
}

TEST_F(ZookeeperCcTest, GetChildren) {
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

TEST_F(ZookeeperCcTest, CreateRestartExists) {
  ASSERT_TRUE(zk_->Init());
  ASSERT_EQ(ZOK, zk_->EnforcePath("some_node", &ZOO_OPEN_ACL_UNSAFE));

  ASSERT_TRUE(StopZookeeper());
  EXPECT_THAT(zk_->Exists("some_node", nullptr, nullptr),
      AnyOf(Eq(ZCONNECTIONLOSS), Eq(ZOPERATIONTIMEOUT)));

  ASSERT_TRUE(StartZookeeper());
  EXPECT_EQ(ZOK, zk_->Exists("some_node", nullptr, nullptr));
}

TEST_F(ZookeeperCcTest, ExistsWatcher) {
  ASSERT_TRUE(zk_->Init());

  std::promise<void> created_promise, deleted_promise;
  Zookeeper::WatcherCallback created_watcher =
      [&created_promise](int type, int state, const char *path) {
        created_promise.set_value();
      };
  Zookeeper::WatcherCallback deleted_watcher =
      [&deleted_promise](int type, int state, const char *path) {
        deleted_promise.set_value();
      };
  std::future<void> created_future = created_promise.get_future();
  std::future<void> deleted_future = deleted_promise.get_future();
  ASSERT_EQ(ZNONODE, zk_->Exists("some_node", &created_watcher, nullptr));

  EXPECT_FALSE(is_ready(created_future));
  ASSERT_EQ(ZOK, zk_->EnforcePath("some_node", &ZOO_OPEN_ACL_UNSAFE));
  EXPECT_TRUE(wait_ready(created_future));
  ASSERT_EQ(ZOK, zk_->Exists("some_node", &deleted_watcher, nullptr));

  EXPECT_FALSE(is_ready(deleted_future));
  ASSERT_EQ(ZOK, zk_->Delete("some_node"));
  EXPECT_TRUE(wait_ready(deleted_future));
  ASSERT_EQ(ZNONODE, zk_->Exists("some_node", nullptr, nullptr));
}

// Ensure that watcher set before restart is triggered if the node is created
// after restart (and disconnection); session events should also trigger the
// watcher.
TEST_F(ZookeeperCcTest, ExistsWatcherOnCreateAfterRestart) {
  ASSERT_TRUE(zk_->Init());

  std::promise<void> disconnected_promise, connected_promise, created_promise;
  Zookeeper::WatcherCallback watcher =
      [&disconnected_promise, &connected_promise, &created_promise]
      (int type, int state, const char *path) {
        if (type == ZOO_SESSION_EVENT) {
          if (state == ZOO_CONNECTING_STATE) {
            disconnected_promise.set_value();
          } else if (state == ZOO_CONNECTED_STATE) {
            connected_promise.set_value();
          }
        }
        if (type == ZOO_CREATED_EVENT) {
          created_promise.set_value();
        }
      };
  std::future<void> disconnected_future = disconnected_promise.get_future();
  std::future<void> connected_future = connected_promise.get_future();
  std::future<void> created_future = created_promise.get_future();
  ASSERT_EQ(ZNONODE, zk_->Exists("some_node", &watcher, nullptr));
  EXPECT_FALSE(is_ready(disconnected_future));
  EXPECT_FALSE(is_ready(connected_future));
  EXPECT_FALSE(is_ready(created_future));

  ASSERT_TRUE(StopZookeeper());
  EXPECT_TRUE(wait_ready(disconnected_future));
  EXPECT_FALSE(is_ready(connected_future));
  EXPECT_FALSE(is_ready(created_future));

  ASSERT_TRUE(StartZookeeper());
  EXPECT_TRUE(wait_ready(connected_future));
  EXPECT_FALSE(is_ready(created_future));

  ASSERT_EQ(ZOK, zk_->EnforcePath("some_node", &ZOO_OPEN_ACL_UNSAFE));
  EXPECT_TRUE(is_ready(created_future));

  ASSERT_EQ(ZOK, zk_->Exists("some_node", nullptr, nullptr));
}

// Ensure that watcher which could not be set while server is down is not
// triggered after node is created after reconnection.
TEST_F(ZookeeperCcTest, ExistsWatcherSetDuringRestart) {
  ASSERT_TRUE(zk_->Init());

  std::promise<void> promise;
  Zookeeper::WatcherCallback watcher =
      [&promise](int type, int state, const char *path) {
        if (type == ZOO_CREATED_EVENT) {
          promise.set_value();
        }
      };
  std::future<void> future = promise.get_future();

  ASSERT_TRUE(StopZookeeper());
  ASSERT_THAT(zk_->Exists("some_node", &watcher, nullptr),
      AnyOf(Eq(ZCONNECTIONLOSS), Eq(ZOPERATIONTIMEOUT)));
  ASSERT_TRUE(StartZookeeper());

  EXPECT_FALSE(is_ready(future));
  ASSERT_EQ(ZOK, zk_->EnforcePath("some_node", &ZOO_OPEN_ACL_UNSAFE));
  EXPECT_FALSE(wait_ready(future));

  ASSERT_EQ(ZOK, zk_->Exists("some_node", nullptr, nullptr));
}
