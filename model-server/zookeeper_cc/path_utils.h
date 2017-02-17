#ifndef ZOOKEEPER_CC_PATH_UTILS_H_
#define ZOOKEEPER_CC_PATH_UTILS_H_

#include <string>

namespace zookeeper_cc {

// Returns modified `data` with added trailing slash,
// unless it was already presented.
std::string EnsureTrailingSlash(std::string data);

// Returns part of the `path` after last slash. If there
// are no slashes in the `path`, returns `path`.
const char* GetLastPathSegment(const char *path);

}  // namespace zookeeper_cc

#endif  // ZOOKEEPER_CC_PATH_UTILS_H_
