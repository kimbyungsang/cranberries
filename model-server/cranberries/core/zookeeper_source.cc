#include "zookeeper_source.h"

#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/lib/strings/numbers.h"
#include "tensorflow_serving/core/servable_data.h"
#include "tensorflow_serving/core/servable_id.h"
#include "zookeeper_cc/path_utils.h"

using tensorflow::strings::StrCat;
using tensorflow::strings::safe_strto64;

// This implementation uses Zookeeper Watches to monitor configuration changes.
// It sets multiple watches on znodes under <base-path>/aspired-models and
// re-reads the tree (or subtree) when something changes.
//
// NOTE: it does not try to avoid setting same watcher on same znode several
// time; this redundancy is expected to be handled by the underlying Zookeeper
// client library: it should avoid issuing same event to the same watch
// function, otherwise exponential increase of number of watches may be
// possible.
//
// This implementation has three main callbacks which set watches and update
// data:
// 1. ReloadAspiredModels()
//   a. Sets watches on "aspired-models" znode and set of its children, they
//      re-trigger ReloadAspiredModels().
//   b. Thus, this callback is run every time this znode is created/removed
//      or set of its children is changed.
//   c. Calls ReloadAspiredModelVersions() for all _new_ children. "Current"
//      children are tracked in the monitored_models_ field. This is done for
//      optimization purposes: we don't want to reload all versions of all
//      models every time. Invariant for monitored_models_: all models in it
//      will be processed ReloadAspiredModelVersions() at some point in the
//      future: either because it will be called directly, or because it's a
//      watch callback. In particular, it's always safe to remove from
//      monitored_models_ and we do not guarantee that it's eventually
//      consistent with Zookeeper's state.
// 2. ReloadAspiredModelVersions()
//   a. Sets watch on set of <base-path>/aspired-models/<model-name>'s children.
//   b. Thus, this callback Re-run every time set of model's znode's children is
//      changed, the znode is deleted.
//   c. It reads list of children of <base-path>/aspired-models/<model-name>,
//      reads their data, and calls aspired_versions_callback_ from Source<>
//      interface.
//   d. It also sets data watch on all correct verions of the model, so if some
//      version's znode had empty data, we will be notified when it's changed.
// 3. ReloadAspiredModelVersion()
//   a. Ran by changes of <base-path>/aspired-models/<model-name>/<version>.
//   b. Simply calls ReloadAspiredModelVersions() to reload list of aspired
//      versions of the corresponding model.
//
// Note that ABA problem does not look like a problem here: we're guaranteed
// to receive an update even for ABAs (except for when a znode was created and
// then removed during disconnection), and then we will look at the most recent
// data and set new watch on the latest available znode.

namespace {

static const char kAspiredModelsZnode[] = "aspired-models";

}

namespace tensorflow {
namespace serving {
namespace cranberries {

ZookeeperSource::ZookeeperSource(zookeeper_cc::Zookeeper *zookeeper)
  : reload_aspired_models_(
      [this](int type, int state, const char* path) {
          ReloadAspiredModels();
      })
  , reload_aspired_model_versions_(
      [this](int type, int state, const char* path) {
          ReloadAspiredModelVersions(path);
      })
  , reload_aspired_model_version_(
      [this](int type, int state, const char* path) {
          ReloadAspiredModelVersion(path);
      })
  , zookeeper_(zookeeper)
{
  // Watch for session changes.
  zookeeper_->SetWatcher(&reload_aspired_models_);
}

void ZookeeperSource::SetAspiredVersionsCallback(
        AspiredVersionsCallback callback) {
  {
    mutex_lock l(mu_);
    set_aspired_versions_callback_ = callback;
  }
  ReloadAspiredModels();
};

void ZookeeperSource::ReloadAspiredModels() {
  // Do not ignore session events, it's the "root" watch which should actually
  // handle state changes.

  // We want to be notified when the node is created, so we set a watch; we
  // don't mind being notified when the node is removed as well.
  int res = zookeeper_->Exists(kAspiredModelsZnode, &reload_aspired_models_,
                               nullptr);
  if (res == ZNONODE) {
    // Probably the node was just removed, waiting until it's created.
    LOG(WARNING) << "Node " << zookeeper_->GetBasePath() << kAspiredModelsZnode << " does not exist";
    return;
  }
  if (res != ZOK) {
    LOG(ERROR) << "Zookeeper error " << res;
    return;
  }

  LOG(INFO) << "Reloading information from " << zookeeper_->GetBasePath() << kAspiredModelsZnode;
  std::vector<std::string> models;
  res = zookeeper_->GetChildren(kAspiredModelsZnode, &models,
                                &reload_aspired_models_);
  if (res == ZNONODE) {
    // Potential race condition: znode can be deleted between Exists() and
    // GetChildren(), not a problem, we will re-set watch once it reappears.
    return;
  }
  if (res != ZOK) {
    LOG(ERROR) << "Zookeeper error " << res;
    return;
  }

  // Start watching new models. First, remove all models which are in the
  // monitored_models_ list, then add the remaining to the list, and finally
  // call ReloadAspiredModelVersions() on newly created models.
  {
    // Lock is scoped because ReloadAspiredModelVersions() will take it. It's
    // ok to release lock and possibly call ReloadAspiredModelVersions() with
    // "slightly inconsistent" monitored_models_ list because the invariant
    // is preserved: after we exit the block, ReloadAspiredModelVersions() is
    // guaranteed to be called for all models in monitored_models_ list at some
    // point in the future.
    mutex_lock l(mu_);
    std::remove_if(models.begin(), models.end(),
        [this](const std::string &model_name) {
          return monitored_models_.count(model_name) > 0;
        }
    );
    monitored_models_.insert(models.begin(), models.end());
  }
  for (const auto &model_name : models) {
    ReloadAspiredModelVersions(model_name.c_str());
  }
}

void ZookeeperSource::ReloadAspiredModelVersions(const char *path) {
  if (!path[0]) {
    // Session event, ignoring.
    return;
  }
  const char *name = zookeeper_cc::GetLastPathSegment(path);

  // Get set of children and set watch for it.
  std::vector<std::string> versions;
  int res = zookeeper_->GetChildren(
    StrCat(kAspiredModelsZnode, "/", name).c_str(),
    &versions,
    &reload_aspired_model_versions_
  );
  if (res == ZNONODE) {
    mutex_lock l(mu_);
    // It's always safe to remove from monitored_models_. Here we have to do it
    // because ZNONODE means that the znode was deleted from Zookeeper, as well
    // as its watches, so we do not monitor it anymore.
    monitored_models_.erase(name);
    return;
  }
  if (res != ZOK) {
    LOG(ERROR) << "Zookeepeer error " << res;
    return;
  }

  std::vector<ServableData<StoragePath>> aspired_versions;
  aspired_versions.reserve(versions.size());
  // Retrieve paths for all valid aspired versions.
  for (const auto &version : versions) {
    ServableId id;
    id.name = name;
    if (!safe_strto64(version.c_str(), &id.version) || id.version < 0) {
      LOG(ERROR) << "Invalid version of model " << name << ": " << version;
      continue;
    }
    std::string path;
    res = zookeeper_->Get(
      StrCat(kAspiredModelsZnode, "/", name, "/", version).c_str(),
      &path,
      &reload_aspired_model_version_,
      nullptr
    );
    if (res != ZOK) {
      LOG(ERROR) << "Zookeeper error " << res;
      continue;
    }
    if (path.empty()) {
      LOG(INFO) << "Model " << name << ", version " << version
                << ": path is empty";
      continue;
    }
    LOG(INFO) << "Model " << name << ", version " << version
              << ": located at " << path;
    aspired_versions.emplace_back(id, std::move(path));
  }
  if (set_aspired_versions_callback_) {
    LOG(INFO) << "Will aspire " << aspired_versions.size()
              << " versions of model " << name;
    set_aspired_versions_callback_(name, aspired_versions);
  }
}

void ZookeeperSource::ReloadAspiredModelVersion(const char *path) {
  if (!path[0]) {
    // Session event, ignoring.
    return;
  }
  std::string model_path = string(path);
  auto version_pos = model_path.find_last_of('/');
  CHECK(version_pos != std::string::npos);
  model_path.erase(model_path.begin() + version_pos, model_path.end());
  ReloadAspiredModelVersions(model_path.c_str());
}

}  // namespace cranberries
}  // namespace serving
}  // namespace tensorflow

