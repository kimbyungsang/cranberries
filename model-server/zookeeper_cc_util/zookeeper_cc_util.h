#ifndef ZOOKEEPER_CC_UTIL_H_
#define ZOOKEEPER_CC_UTIL_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "zookeeper.h"

namespace zookeeper_cc_util {

// Returns modified `data` with added trailing slash,
// unless it was already presented.
std::string EnsureTrailingSlash(std::string data);

// Returns part of the `path` after last slash. If there
// are no slashes in the `path`, returns `path`.
const char* GetLastPathSegment(const char *path);

// RAII wrapper around zhandle_t for talking to Zookeeper. Allows to specify
// "base" path which will be automatically prepended to all requests. Note that
// callbacks will receive full paths, including "base" path.
class Zookeeper {
 public:
  // Instantiate wrapper, do not create zhandle_t and do not make
  // connection to Zookeeper.
  Zookeeper(std::string hosts, int recv_timeout, std::string base);
  ~Zookeeper() {}

  // Tries to create zhandle_t and make connection to Zookeeper.
  // Returns true if zhandle_t creation was successful (note that
  // connection can still be in progress).
  bool Init();

  using WatcherCallback = std::function<void(int, int, const char*)>;

  // Following routines call corresponding blocking zoo_* routines
  // and returns its status: ZOK, etc.
  int Create(const char *path, const std::string &value,
             bool ephemeral, struct ACL_vector *acl);
  int Exists(const char *path, const WatcherCallback *watcher,
             struct Stat *stat);
  int Get(const char *path, std::string *output,
          const WatcherCallback *watcher, struct Stat *stat);
  int Set(const char *path, const std::string &data, int version = -1);
  int Delete(const char *path, int version = -1);
  int GetChildren(const char *path, std::vector<std::string> *children_name,
                  const WatcherCallback *watcher);

  // Tries to create all znodes in path from top to bottom. If some node
  // already exists, it's not touched. Nodes are created with empty content.
  int EnforcePath(const char *path, struct ACL_vector *acl);

 private:
  struct ZhandleDeleter {
    void operator()(zhandle_t *zh) { zookeeper_close(zh); }
  };

  static void WatcherHandler(zhandle_t *zzh, int type, int state,
                             const char *path, void *watcherCtx);

  std::string GetAbsolutePath(const char *path);

  std::unique_ptr<zhandle_t, ZhandleDeleter> zh_;
  const std::string hosts_;
  const int recv_timeout_;
  const std::string base_;
};

}  // namespace zookeeper_cc_util

#endif  // ZOOKEEPER_CCC_UTIL_H_
