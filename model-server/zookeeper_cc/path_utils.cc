#include "path_utils.h"

#include <string.h>

namespace zookeeper_cc {

std::string EnsureTrailingSlash(std::string data) {
  if (!data.empty() && data[data.size() - 1] == '/') {
    return data;
  } else {
    return data + "/";
  }
}

const char* GetLastPathSegment(const char *path) {
  const char *last_sep = strrchr(path, '/');
  if (!last_sep) {
    return path;
  } else {
    return last_sep + 1;
  }
}

}  // namespace zookeeper_cc
