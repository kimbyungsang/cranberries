#ifndef CRANBERRIES_ZOOKEEPER_SOURCE_H_
#define CRANBERRIES_ZOOKEEPER_SOURCE_H_

#include <unordered_set>
#include "tensorflow/core/platform/mutex.h"
#include "tensorflow_serving/core/source.h"
#include "tensorflow_serving/core/storage_path.h"
#include "zookeeper_cc/zookeeper_cc.h"

namespace tensorflow {
namespace serving {
namespace cranberries {

// ZookeeperSouce is an implementation Source<StoragePath> interface which
// monitors Zookeeper for znodes in the following format:
// <base-path>/aspired-models/<model-name>/<model-version-id>
// Here <base-path> prepending is handled by `zookeeper_cc::Zookeper`.
//
// The following structure inside Zookeeper is assumed:
// <base-path>
// +-- aspired-models (no data)
//     | list of models:
//     +-- model1 (no data)
//     |   | list of model's versions:
//     |   +-- 1 (data contains path to the version)
//     |   +-- 2 (similar)
//     +-- model2 (no data)
//         | list of model's versions:
//         +-- 1 (data contains path to the versions)
//         +-- 2 (similar)
//
// This configuration is tracked in real-time, one should be able to perform
// arbitrary changes, including deletion of <base-path> and re-creation from
// scratch. In order to simplify usage of third-party tools which does not
// support atomic "create and set data" operation, this implementation ignores
// version znodes which do not contain empty data - they are not included in
// the list of aspired versions.
//
// Nevertheless, it's not allowed to alter znode's data after it was seen by
// ZookeeperSource, because of limitation of AspiredVersionsManager: it assumes
// that paths do not change, thus it does not check whether path of an already
// loaded Servable (e.g. model) was changed and will not reload it. If you want
// to change the model's version's path, you'd better create new version. Or, if
// you really want, you may remove the node, wait until the change is propagated
// and the old model is unloaded, and the create the new node. Obviously, that
// will cause some downtime for that model.
//
// NOTE: AspiredVersionsManager "queues" load requests and do not process
// further model load requests until previous are processed and retried
// several times. That means that if you specify invalid path to model's
// version, further changes won't be reflected until AspiredVersionsManager
// decides to stop retrying to load the model from invalid path. Similarly,
// if a model takes long time to load, all further changes won't be reflected
// until the loading is finished.
//
// TODO(egor.suvorov): handle re/disconnections from Zookeeper because of
// session expirations (Zookeeper* becomes invalid?).
// TODO(egor.suvorov): handle Zookeeper errors gracefully without losing
// watches (if that's an issue at all).
// TODO(egor.suvorov): make it asynchronous(?).
class ZookeeperSource : public Source<StoragePath> {
 public:
  ZookeeperSource(zookeeper_cc::Zookeeper *zookeeper);
  ~ZookeeperSource() {}

  void SetAspiredVersionsCallback(AspiredVersionsCallback callback) override;

 private:
  mutable mutex mu_;

  using WatcherCallback = zookeeper_cc::Zookeeper::WatcherCallback;

  void ReloadAspiredModels();
  void ReloadAspiredModelVersions(const char *path);
  void ReloadAspiredModelVersion(const char *path);

  const WatcherCallback reload_aspired_models_;
  const WatcherCallback reload_aspired_model_versions_;
  const WatcherCallback reload_aspired_model_version_;

  zookeeper_cc::Zookeeper *zookeeper_ GUARDED_BY(mu_);
  AspiredVersionsCallback set_aspired_versions_callback_ GUARDED_BY(mu_);
  std::unordered_set<string> monitored_models_ GUARDED_BY(mu_);

  TF_DISALLOW_COPY_AND_ASSIGN(ZookeeperSource);
};

}  // namespace cranberries
}  // namespace serving
}  // namespace tensorflow

#endif  // CRANBERRIES_ZOOKEEPER_SOURCE_H_
