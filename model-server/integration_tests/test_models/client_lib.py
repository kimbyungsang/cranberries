import grpc
import tensorflow as tf
from tensorflow_serving.apis import predict_pb2
from tensorflow_serving.apis import prediction_service_pb2


def make_request(server, input_value, model, version=None):
    channel = grpc.insecure_channel(server)
    stub = prediction_service_pb2.PredictionServiceStub(channel)

    request = predict_pb2.PredictRequest()
    request.model_spec.name = model
    if version:
        request.model_spec.version.value = version

    request.inputs["inp"].CopyFrom(
        tf.contrib.util.make_tensor_proto([input_value], shape=[1]))
    response = stub.Predict(request)
    [result_value] = tf.contrib.util.make_ndarray(response.outputs["out"])
    return result_value
