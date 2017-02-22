# Building

1. You should check out all submodules recursively, as this repository
   includes [TensorFlow Serving](https://github.com/tensorflow/serving) as a
   submodule (which, in turn, includes a lot of other stuff). To do this, run:

   ~~~shell
   git submodule update --init --recursive
   ~~~
   
   This step will download several hundered megabytes of data.

2. Then you should install all prerequisites for TensorFlow Serving, as listed
on their [Installation page](https://tensorflow.github.io/serving/setup#prerequisites).
In particular, you should:
  1. Install [Bazel](https://bazel.build) >=0.4.4.
  2. Install building dependencies (e.g. `libpng`).
  3. Go to `<repo-root>/serving/tensorflow` and run `./configure` script from
     here. It's ok to leave all settings in their defaults for local set-up.
3. Install prerequisites for Apache Zookeeper. I don't know what they are,
   but I've found only one which can be missing so far:

   ~~~shell
   sudo apt-get install realpath
   ~~~
4. Run `bazel build //cranberries/model_server` from `model-server`
   directory to build and run the server. Building may take a while because
   Bazel will download even more dependencies and compile everything from
   sources.

# Running tests

1. You should install [Docker](https://www.docker.com/) in addition to Bazel to
   run tests, as some of them run real Apache Zookeeper server inside Docker.
2. You should also install [Kazoo](https://kazoo.readthedocs.io/en/latest/)
   library for Python2. There is a chance it's already installed: if
   `import kazoo' statement does not fail, you're probably good to go.
   If it does, install Kazoo with either:
   * `sudo -H pip install kazoo`
   * `sudo apt-get install python-kazoo`
3. Run

   ~~~shell
   bazel test //...
   ~~~

   to run all tests (here `...` stands for "all targets in this package and
   subpackages"). First run may take some time to download Zookeeper docker
   image, it should be faster afterwards.

# Usage

## Initial configuration

1. Install [Apache Zookeeper](http://zookeeper.apache.org/) (either single
   server or cluster configurations are ok).
   [Docker image](https://hub.docker.com/_/zookeeper/) is also available, and
   I find it quite handy for local development; its default settings are fine.
2. Choose a base path in your Zookeeper instance where all data belonging to
   Cranberries model server will live. I suggest something like this:

   ~~~
   /cranberries/servers/yeputons-desktop
   ~~~
3. Assuming that you have a single Zookeeper server running on
   `172.17.0.2:2181`, you can run Cranberries model server with this command
   typed inside `model-server` directory:

   ~~~shell
   bazel run //cranberries/model_server -- --zookeeper_hosts=172.17.0.2:2181 --zookeeper_base=/cranberries/servers/yeputons-dekstop
   ~~~

   Alternatively, you can call the compiled executable directly:
   ~~~shell
   ./bazel-bin/cranberries/model_server/model_server --zookeeper_hosts=172.17.0.2:2181 --zookeeper_base=/cranberries/servers/yeputons-desktop
   ~~~

   There may be other options (like `--port`), run `model_server` without
   parameters to see them.
4. You should see a bunch of informational messages, including
   `Running Model Server at ...`.

## Managing models

If you want to load a new model into model server, you should:

1. Save it in TensorFlow's `SavedModel` format (see [tutorial](https://tensorflow.github.io/serving/serving_basic)
   for details). You should end up with a folder containing
   two files: `saved_model.pb` and `variables`. Let's assume that this folder
   is called `/tmp/mnist_model/2`, model is called `mnist` and its version is 2.
2. Create the following znode in Zookeeper (assuming that you have base path
   from the previous section):

   ~~~
   /cranberries/servers/yeputons-desktop/aspired-models/mnist/2
   ~~~

   with the following data:
   ~~~
   /tmp/mnist_model/2
   ~~~

   You're free to either create the znode and fill it with data atomically or,
   if your client does not support that, create the znode with empty data and
   set its data later. Model server will use the first non-empty data it sees.
   Note that you cannot modify path of a version which is already loaded.
3. If you want to load other versions, simply repeat step 2 for them. You
   should see log messages about picking up changes from Zookeeper.
4. After model is seen by model server, its state is reflected in ephemeral
   znodes under `current-models` znode, e.g. this znode

   ~~~
   /cranberries/servers/yeputons-desktop/current-models/mnist/2
   ~~~

   may have the following data:

   ~~~
   kAvailable
   ~~~

  which means that the model was successfully loaded into TensorFlow Serving.
  Full list of states can be found in [`servable_state.h`](https://github.com/tensorflow/serving/blob/48dfb5581a621c7a6e038df3415e0eff02edb043/tensorflow_serving/core/servable_state.h#L38-L64).
5. Note that that all changes in Zookeeper are processed synchronously and
   one-by-one. E.g. if some model fails to load, TensorFlow Serving will wait
   a minute (default value) and then try to re-load it, several times. During
   that time, no other changes to the configuration will be performed (they
   will be monitored and queued, though).
6. When you want to unload a specific version of a model, just remove
   corresponding node, e.g.:

   ~~~
   /cranberries/servers/yeputons-desktop/aspired-models/mnist/2
   ~~~

   Corresponding state under `current-models` will change to `kUnloading`
   and then the znode will be removed.
