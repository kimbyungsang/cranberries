# Installation

1. You should check out all submodules recursively, as this repository
   includes [TensorFlow Serving](https://github.com/tensorflow/serving) as a
   submodule (which, in turn, includes a lot of other stuff). To do this, run:

   ~~~shell
   git submodule update --init --recursive
   ~~~

2. Then you should install all prerequisites for TensorFlow Serving, as listed
on their [Installation page](https://tensorflow.github.io/serving/setup#prerequisites).
In particular, you should:
  1. Install [Bazel](https://bazel.build).
  2. Install building dependencies (e.g. `libpng`).
  3. Go to `./serving/tensorflow` and run `./configure` script from here.
     It's ok to leave all settings in their defaults for local set-up.
3. Run `bazel run //cranberries/model_server` to build and run server. Building
   may take a while because Bazel will download multiple dependencies and
   compile everything from sources.
4. After successful compilation and running you should see usage information,
   like this:

   ~~~
   ...
   --zookeeper_hosts="localhost:2181"      string  Specify list of Zookeeper servers in ensemble in format host1:port1,host2:port2,host3:port3.
   ...
   ~~~
