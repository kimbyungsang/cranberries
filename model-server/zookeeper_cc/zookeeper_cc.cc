#include "zookeeper_cc.h"

#include <utility>
#include <assert.h>
#include "path_utils.h"

namespace zookeeper_cc {

Zookeeper::Zookeeper(std::string hosts, int recv_timeout, std::string base)
  : hosts_(std::move(hosts))
  , recv_timeout_(recv_timeout)
  , base_(EnsureTrailingSlash(std::move(base)))
{
}

std::string Zookeeper::GetAbsolutePath(const char *path) {
  if (path[0] == '/') {
    return base_ + (path + 1);
  } else if (!path[0]) {
    return base_.substr(0, base_.size() - 1);
  } else {
    return base_ + path;
  }
}

bool Zookeeper::Init() {
  assert(!zh_);
  zh_.reset(zookeeper_init(
    hosts_.c_str(),
    &Zookeeper::WatcherHandler,
    recv_timeout_,
    NULL, // clientid
    NULL, // watcher context
    0 // flags
  ));
  return static_cast<bool>(zh_);
}

void Zookeeper::SetWatcher(const WatcherCallback *watcher) {
  assert(zh_);
  zoo_set_context(zh_.get(), const_cast<void*>(static_cast<const void*>(watcher)));
}

int Zookeeper::Create(const char *path, const std::string &value,
                      bool ephemeral, struct ACL_vector *acl) {
  assert(zh_);
  return zoo_create(
    zh_.get(),
    GetAbsolutePath(path).c_str(),
    value.data(),
    value.size(),
    acl,
    (ephemeral ? ZOO_EPHEMERAL : 0), // flags
    NULL, // path_buffer
    0 // path_buffer_len
  );
}

int Zookeeper::Exists(const char *path, const WatcherCallback *watcher,
                      struct Stat *stat) {
  assert(zh_);
  return zoo_wexists(
    zh_.get(),
    GetAbsolutePath(path).c_str(),
    watcher ? &Zookeeper::WatcherHandler : NULL,
    const_cast<void*>(static_cast<const void*>(watcher)),
    stat
  );
}

int Zookeeper::Get(const char *path, std::string *output,
                   const WatcherCallback *watcher, struct Stat *stat) {
  assert(zh_);
  // Zookeeper's limit on data size is 1 MiB, plus 1 for null terminator.
  static const int BUF_SIZE = 1024 * 1024 + 1;
  thread_local char buf[BUF_SIZE];
  int len = sizeof buf;
  int res = zoo_wget(
    zh_.get(),
    GetAbsolutePath(path).c_str(),
    watcher ? &Zookeeper::WatcherHandler : NULL,
    const_cast<void*>(static_cast<const void*>(watcher)),
    buf,
    &len,
    stat
  );
  if (res == ZOK && output) {
    output->assign(buf, buf + len);
  }
  return res;
}

int Zookeeper::Set(const char *path, const std::string &data, int version) {
  assert(zh_);
  return zoo_set(zh_.get(), GetAbsolutePath(path).c_str(),
                 data.data(), data.size(), version);
}

int Zookeeper::Delete(const char *path, int version) {
  assert(zh_);
  return zoo_delete(zh_.get(), GetAbsolutePath(path).c_str(), version);
}

int Zookeeper::GetChildren(const char *path,
                           std::vector<std::string> *children_name,
                           const WatcherCallback *watcher) {
  assert(zh_);
  String_vector children {};
  int res = zoo_wget_children(
    zh_.get(),
    GetAbsolutePath(path).c_str(),
    watcher ? &Zookeeper::WatcherHandler : NULL,
    const_cast<void*>(static_cast<const void*>(watcher)),
    &children
  );
  if (res == ZOK) {
    children_name->assign(children.data, children.data + children.count);
  }
  deallocate_String_vector(&children);
  return res;
}

int Zookeeper::EnforcePath(const char *rel_path, struct ACL_vector *acl) {
  assert(zh_);
  std::string full_path = EnsureTrailingSlash(GetAbsolutePath(rel_path));
  int res = ZOK;
  // Iterate over all parent znodes of `full_path` by replacing slashes
  // with null characters one-by-one, skipping the very first slash at
  // position 0 (root znode already exists).
  for (size_t i = 1; i < full_path.size(); i++) {
    if (full_path[i] == '/') {
      full_path[i] = 0;
      res = zoo_create(
        zh_.get(),
        full_path.c_str(),
        "", // value
        0, // valuelen
        acl,
        0, // flags
        NULL, // path_buffer
        0 // path_buffer_length
      );
      full_path[i] = '/';
      if (res != ZOK && res != ZNODEEXISTS) {
        return res;
      }
    }
  }
  return res;
}

int Zookeeper::State() {
  return zoo_state(zh_.get());
}

void Zookeeper::WatcherHandler(zhandle_t *zzh, int type, int state,
                 const char *path, void *watcherCtx) {
  if (!watcherCtx) {
    return;
  }
  (*static_cast<WatcherCallback*>(watcherCtx))(type, state, path);
}

}  // namespace zookeeper_cc
