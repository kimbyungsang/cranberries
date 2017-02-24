# Test models

This package contains tooling for some very simple models, which are aimed for
testing. Each model takes number `x` as an input and produces `x+k1+k2` as an
output, where:
* `k1` is 100 for model `a`, 200 for model `b`.
* `k2` is 10 for version 1 of both models, 20 for version 20 of both models.
This yields 4 packages which are formed.

## Generation
Run `bazel build :test_models` from this directory to generate archive
`test_models.tar` with the following structure:

~~~
test_models.tar
+-- models/
    +-- a/
        +-- 1/
        +-- 2/
    +-- b/
        +-- 1/
        +-- 2/
~~~

You can untar this to any directory, point `model_server` to the `models`
directory and start serving.

## Example
Assuming that this repository is located at `~/cranberries`, all prerequisites
for Cranberries are installed, and we are currently in the
`<repo-root>/model-server` directory, here is a list of commands which will set
up the server and run test clients:

~~~shell
mkdir ~/test-models-demo
bazel build //integration_tests/test_models
tar xvpf bazel-genfiles/integration_tests/test_models/test_models.tar -C ~/test-models-demo
bazel build @tf_serving//tensorflow_serving/model_servers:tensorflow_model_server
bazel-bin/external/tf_serving//tensorflow_serving/model_servers/tensorflow_model_server --model_name=a --model_base_path="$HOME/test-models-demo/models/a"
# Run the commands in a separate shell
# Should print 125
bazel run //integration_tests/test_models:client -- --server localhost:8500 --model a --input_value 5
# Should fail, because default TensorFlow Serving model server loads only the
# latest version of the model
bazel run //integration_tests/test_models:client -- --server localhost:8500 --model a --input_value 5 --version 1
~~~
