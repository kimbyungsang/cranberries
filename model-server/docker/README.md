# Building and running single image

As we don't have any pre-built image available, you should build one yourself.
This was tested on Ubuntu 16.04.

1. Build model server using instructions from the parent folder. It should end
   with `bazel build //cranberries/model_server` command.
2. From this directory, run:

   ~~~shell
   cp ../bazel-bin/cranberries/model_server/model_server .
   ~~~
3. Run:
   ~~~shell
   docker build .
   ~~~
4. Docker should end its output with line like:

   ~~~
   Successfully built 8f4049b2df36
   ~~~

   that hash is your image's id.
5. You can create an alias for your image with the following command:

   ~~~shell
   docker tag 8f4049b2df36 cranberries-model-server
   ~~~
6. Afterwards you can start the container like this:

   ~~~shell
   docker run -e ZOOKEEPER_HOSTS=172.17.0.2:2181 -e ZOOKEEPER_BASE=/cranberries/servers/dockered cranberries-model-server
   ~~~

   remember to replace `172.17.0.2:2181` with your Zookeeper's server address
   and `/cranberries/servers/dockered` with your real Zookeeper base path.

# Docker Compose
Example `docker-compose.yml` contains:
1. Cluster of three Zookeeper servers.
2. [Zookeeper Web UI](https://github.com/tobilg/docker-zookeeper-webui) which
   listens on port 8080.
3. Model server which listens on port 8050.

## Running servers in foreground
1. Run `docker-compose up`.
2. You should see servers starting and printing logs in terminal in different
   colors.
3. To stop servers, press Ctrl+C and wait until all servers are gracefully
   terminated.
4. If anything goes wrong, I recommend typing `docker-compose down` afterwards
   to ensure there are no outstanding containers.

## Running servers in background
1. Run `docker-compose up -d` and wait until all containers are started.
2. Do your stuff.
3. Run `docker-compose down` to terminate all containers.

## Managing Zookeeper
1. Go to [localhost:8080](http://localhost:8080) after starting the orchestra.
2. Connection string like `zoo1:2181,...` should already be on the screen,
   click on it.
3. You will enter read-only mode.
4. In order to be able to edit data, click "Login" in the top-right corner
   and use user `admin` with password `admin` to log in.
5. Modification buttons are located on the bottom of the UI. "RMR" stands for
   `rm -r` (recursive deletion of nodes).

## Managing models
1. The `./volumes/models` folder is mounted as `/models` inside model server
container. So if you put your model under `./volumes/models/mnist_model/1`,
you should specify `/models/mnist_model/1` as its loading path in Zookeeper.
2. Model server's base path is `/cranberries/servers/docker-composed`. So if
   you want to add MNIST model, you can create the following node:
   ~~~
   /cranberries/servers/docker-composed/aspired-models/mnist/1
   ~~~
   with the following data:
   ~~~
   /models/mnist_model/1
   ~~~
3. In the next few seconds model server should load the model and create the following node:
   ~~~
   cranberries/servers/docker-composed/current-models/mnist/1
   ~~~
   with the following data:
   ~~~
   kAvailable
   ~~~

## Troubleshooting
* If you see that `model_server` does not start for some reason, ensure that
its Docker image was built correctly (see above). If you have to change
`Dockerfile`, run `docker-compose build` to rebuild the image
(`docker build` is not enough for Docker Compose).
* If you want to use different `model_server`, run Bazel to compile it, copy the binary to the current folder and run `docker-compose build` to rebuild the image (`docker build` is not enough).
