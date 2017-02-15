#include "zookeeper_state_reporter.h"

#include <functional>
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/lib/strings/strcat.h"

using tensorflow::serving::ServableState;
using tensorflow::strings::StrCat;
using tensorflow::string;

namespace {

string ManagerStateToString(ServableState::ManagerState manager_state) {
  switch (manager_state) {
  case ServableState::ManagerState::kStart: return "kStart";
  case ServableState::ManagerState::kLoading: return "kLoading";
  case ServableState::ManagerState::kAvailable: return "kAvailable";
  case ServableState::ManagerState::kUnloading: return "kUnloading";
  case ServableState::ManagerState::kEnd: return "kEnd";
  default: return StrCat("UNKNOWN ", static_cast<int>(manager_state));
  }
}

}  // namespace

namespace tensorflow {
namespace serving {
namespace cranberries {

EventBus<ServableState>::Callback ZookeeperStateReporter::GetEventBusCallback() {
  return std::bind(&ZookeeperStateReporter::ProcessEvent, this, std::placeholders::_1);
}

void ZookeeperStateReporter::ProcessEvent(const EventBus<ServableState>::EventAndTime &ev) {
  const auto &state = ev.event;
  string prefix = StrCat("current-models/", state.id.name);
  string name = StrCat(prefix, "/", state.id.version);
  string value = ManagerStateToString(state.manager_state);
  LOG(INFO) << "Reporting servable state to Zookeeper: " << name << "=" << value;

  if (state.manager_state == ServableState::ManagerState::kEnd) {
    int res = zookeeper_->Delete(name.c_str());
    if (res == ZOK || res == ZNONODE) {
      // Do nothing.
    } else {
      LOG(ERROR) << "Zookeeper::Delete() returned " << res;
    }
    return;
  }

  int res = zookeeper_->EnforcePath(prefix.c_str(), &ZOO_OPEN_ACL_UNSAFE);
  if (res != ZOK && res != ZNODEEXISTS) {
    LOG(ERROR) << "Unable to create node in Zookeeper, error code is " << res;
    return;
  }

  res = zookeeper_->Create(name.c_str(), value, true /* ephemeral */, &ZOO_OPEN_ACL_UNSAFE);
  if (res == ZOK) {
    return;
  }
  if (res != ZNODEEXISTS) {
    LOG(ERROR) << "Zookeeper::Create() returned " << res;
    return;
  }
  res = zookeeper_->Set(name.c_str(), value);
  if (res != ZOK) {
    LOG(ERROR) << "Zookeeper::Set() returned " << res;
    return;
  }
}

}  // namespace cranberries
}  // namespace tensorflow
}  // namespace serving
