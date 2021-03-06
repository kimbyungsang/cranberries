// Based on TensorFlow Serving's gRPC server implementation from
// https://github.com/tensorflow/serving/blob/master/tensorflow_serving/model_servers/main.cc
//
// gRPC server implementation of
// tensorflow_serving/apis/prediction_service.proto.
//
// It bring up a standard server which connects to Apache Zookeeper and
// gets dynamic configuration from there; describing which versions of
// which models have to be loaded from what places.
//
// ModelServer has inter-request batching support built-in, by using the
// BatchingSession at:
//     tensorflow_serving/batching/batching_session.h
//
// For detailed description of example Zookeeper configuration, see
//     zookeeper_source.cc
//
// To specify port (default 8500): --port=my_port
// To enable batching (default disabled): --enable_batching

#include <unistd.h>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "google/protobuf/wrappers.pb.h"
#include "grpc++/security/server_credentials.h"
#include "grpc++/server.h"
#include "grpc++/server_builder.h"
#include "grpc++/server_context.h"
#include "grpc++/support/status.h"
#include "grpc++/support/status_code_enum.h"
#include "grpc/grpc.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/protobuf.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/protobuf/config.pb.h"
#include "tensorflow/core/util/command_line_flags.h"
#include "tensorflow_serving/apis/prediction_service.grpc.pb.h"
#include "tensorflow_serving/apis/prediction_service.pb.h"
#include "tensorflow_serving/config/model_server_config.pb.h"
#include "tensorflow_serving/core/availability_preserving_policy.h"
#include "tensorflow_serving/model_servers/model_platform_types.h"
#include "tensorflow_serving/model_servers/platform_config_util.h"
#include "tensorflow_serving/model_servers/server_core.h"
#include "tensorflow_serving/servables/tensorflow/get_model_metadata_impl.h"
#include "tensorflow_serving/servables/tensorflow/saved_model_bundle_source_adapter.h"
#include "cranberries/model_server/predict_impl.h"
#include "cranberries/model_server/model_server_config.pb.h"
#include "zookeeper_cc/zookeeper_cc.h"
#include "cranberries/core/zookeeper_source.h"
#include "cranberries/core/zookeeper_state_reporter.h"

using tensorflow::serving::AspiredVersionsManager;
using tensorflow::serving::AspiredVersionPolicy;
using tensorflow::serving::AvailabilityPreservingPolicy;
using tensorflow::serving::BatchingParameters;
using tensorflow::serving::EventBus;
using tensorflow::serving::GetModelMetadataImpl;
using tensorflow::serving::SavedModelBundleSourceAdapter;
using tensorflow::serving::SessionBundleSourceAdapterConfig;
using tensorflow::serving::ServableState;
using tensorflow::serving::ServerCore;
using tensorflow::serving::SessionBundleConfig;
using tensorflow::serving::TensorflowPredictor;
using tensorflow::serving::UniquePtrWithDeps;
using tensorflow::string;

using grpc::InsecureServerCredentials;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using tensorflow::serving::GetModelMetadataRequest;
using tensorflow::serving::GetModelMetadataResponse;
using tensorflow::serving::PredictRequest;
using tensorflow::serving::PredictResponse;
using tensorflow::serving::PredictionService;
using tensorflow::Status;

using cranberries::ModelServerConfig;
using zookeeper_cc::Zookeeper;
using tensorflow::serving::cranberries::ZookeeperSource;
using tensorflow::serving::cranberries::ZookeeperStateReporter;

namespace {

tensorflow::Status LoadCustomModelConfig(
    const ::google::protobuf::Any& any,
    EventBus<ServableState>* servable_event_bus,
    UniquePtrWithDeps<AspiredVersionsManager>* manager) {
  ModelServerConfig config;
  CHECK(any.UnpackTo(&config));

  std::unique_ptr<Zookeeper> zookeeper(
      new Zookeeper(config.zookeeper_hosts(), 2000 /* recv_timeout */, config.zookeeper_base()));
  CHECK(zookeeper->Init()) << "Unable to init Zookeeper client";

  std::unique_ptr<ZookeeperStateReporter> state_reporter(
      new ZookeeperStateReporter(zookeeper.get()));
  std::unique_ptr<EventBus<ServableState>::Subscription> subscription =
      servable_event_bus->Subscribe(state_reporter->GetEventBusCallback());

  std::unique_ptr<SavedModelBundleSourceAdapter> bundle_adapter;
  SessionBundleSourceAdapterConfig bundle_adapter_config;
  TF_CHECK_OK(SavedModelBundleSourceAdapter::Create(
      bundle_adapter_config, &bundle_adapter));
  ConnectSourceToTarget(bundle_adapter.get(), manager->get());

  std::unique_ptr<ZookeeperSource> source(
      new ZookeeperSource(zookeeper.get()));
  ConnectSourceToTarget(source.get(), bundle_adapter.get());

  manager->AddDependency(std::move(zookeeper));
  manager->AddDependency(std::move(state_reporter));
  manager->AddDependency(std::move(subscription));
  manager->AddDependency(std::move(bundle_adapter));
  manager->AddDependency(std::move(source));
  return Status::OK();
}

grpc::Status ToGRPCStatus(const tensorflow::Status& status) {
  const int kErrorMessageLimit = 1024;
  string error_message;
  if (status.error_message().length() > kErrorMessageLimit) {
    error_message =
        status.error_message().substr(0, kErrorMessageLimit) + "...TRUNCATED";
  } else {
    error_message = status.error_message();
  }
  return grpc::Status(static_cast<grpc::StatusCode>(status.code()),
                      error_message);
}

class PredictionServiceImpl final : public PredictionService::Service {
 public:
  explicit PredictionServiceImpl(std::unique_ptr<ServerCore> core,
                                 bool use_saved_model)
      : core_(std::move(core)),
        predictor_(new TensorflowPredictor(use_saved_model)),
        use_saved_model_(use_saved_model) {}

  grpc::Status Predict(ServerContext* context, const PredictRequest* request,
                       PredictResponse* response) override {
    const grpc::Status status =
        ToGRPCStatus(predictor_->Predict(core_.get(), *request, response));
    if (!status.ok()) {
      VLOG(1) << "Predict failed: " << status.error_message();
    }
    return status;
  }

  grpc::Status GetModelMetadata(ServerContext* context,
                                const GetModelMetadataRequest* request,
                                GetModelMetadataResponse* response) override {
    if (!use_saved_model_) {
      return ToGRPCStatus(tensorflow::errors::InvalidArgument(
          "GetModelMetadata API is only available when use_saved_model is "
          "set to true"));
    }
    const grpc::Status status =
        ToGRPCStatus(GetModelMetadataImpl::GetModelMetadata(
            core_.get(), *request, response));
    if (!status.ok()) {
      VLOG(1) << "GetModelMetadata failed: " << status.error_message();
    }
    return status;
  }

 private:
  std::unique_ptr<ServerCore> core_;
  std::unique_ptr<TensorflowPredictor> predictor_;
  bool use_saved_model_;
};

void RunServer(int port, std::unique_ptr<ServerCore> core,
               bool use_saved_model) {
  // "0.0.0.0" is the way to listen on localhost in gRPC.
  const string server_address = "0.0.0.0:" + std::to_string(port);
  PredictionServiceImpl service(std::move(core), use_saved_model);
  ServerBuilder builder;
  std::shared_ptr<grpc::ServerCredentials> creds = InsecureServerCredentials();
  builder.AddListeningPort(server_address, creds);
  builder.RegisterService(&service);
  builder.SetMaxMessageSize(tensorflow::kint32max);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  LOG(INFO) << "Running ModelServer at " << server_address << " ...";
  server->Wait();
}

}  // namespace

int main(int argc, char** argv) {
  tensorflow::int32 port = 8500;
  bool enable_batching = false;
  tensorflow::string zookeeper_hosts = "localhost:2181";
  tensorflow::string zookeeper_base;
  // Tensorflow session parallelism of zero means that both inter and intra op
  // thread pools will be auto configured.
  tensorflow::int64 tensorflow_session_parallelism = 0;
  std::vector<tensorflow::Flag> flag_list = {
      tensorflow::Flag("port", &port, "port to listen on"),
      tensorflow::Flag("enable_batching", &enable_batching, "enable batching"),
      tensorflow::Flag("zookeeper_hosts", &zookeeper_hosts,
                       "Specify list of Zookeeper servers in ensemble in format "
                       "host1:port1,host2:port2,host3:port3."),
      tensorflow::Flag("zookeeper_base", &zookeeper_base,
                       "Specify path to the base node for this instance of "
                       "TensorFlow Serving, e.g. /cranberries/servers/server-42 "
                       "(required)."),
      tensorflow::Flag("tensorflow_session_parallelism",
                       &tensorflow_session_parallelism,
                       "Number of threads to use for running a "
                       "Tensorflow session. Auto-configured by default.")};
  string usage = tensorflow::Flags::Usage(argv[0], flag_list);
  const bool parse_result = tensorflow::Flags::Parse(&argc, argv, flag_list);
  if (!parse_result || zookeeper_base.empty()) {
    std::cout << usage;
    return -1;
  }
  tensorflow::port::InitMain(argv[0], &argc, &argv);
  if (argc != 1) {
    std::cout << "unknown argument: " << argv[1] << "\n" << usage;
    return -1;
  }

  // For ServerCore Options, we leave servable_state_monitor_creator unspecified
  // so the default servable_state_monitor_creator will be used.
  ServerCore::Options options;
  {
    ModelServerConfig config;
    config.set_zookeeper_hosts(zookeeper_hosts);
    config.set_zookeeper_base(zookeeper_base);
    options.model_server_config.mutable_custom_model_config()->PackFrom(config);
  }

  SessionBundleConfig session_bundle_config;
  // Batching config
  if (enable_batching) {
    BatchingParameters* batching_parameters =
        session_bundle_config.mutable_batching_parameters();
    batching_parameters->mutable_thread_pool_name()->set_value(
        "model_server_batch_threads");
  }

  // use_saved_model is fixed to `true` because `LoadCustomModelConfig`
  // uses SavedModelBundleSourceAdapter only for now, and it's hard-coded.

  session_bundle_config.mutable_session_config()
      ->set_intra_op_parallelism_threads(tensorflow_session_parallelism);
  session_bundle_config.mutable_session_config()
      ->set_inter_op_parallelism_threads(tensorflow_session_parallelism);
  options.platform_config_map = CreateTensorFlowPlatformConfigMap(
      session_bundle_config, true /* use_saved_model */);

  options.custom_model_config_loader = &LoadCustomModelConfig;

  options.aspired_version_policy =
      std::unique_ptr<AspiredVersionPolicy>(new AvailabilityPreservingPolicy);

  std::unique_ptr<ServerCore> core;
  TF_CHECK_OK(ServerCore::Create(std::move(options), &core));
  RunServer(port, std::move(core), true /* use_saved_model */);

  return 0;
}
