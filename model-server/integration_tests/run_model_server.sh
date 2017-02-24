#!/bin/bash

set -u -e
source sh_utils/sh_utils.sh

# TODO(egor.suvorov): start model_server on a random available port, like in
# https://github.com/pagespeed/mod_pagespeed/blob/d55c2d7a2c64fab183f77571a18dad80ffe3cb1b/install/start_background_server.sh#L93
# That will allow to run integration tests in parallel and truly indepepdently
# from each other and current model_server instances running. We do not have
# similar problem with Zookeeper, because it's run in Docker container on a
# separate IP address.

EXEC="cranberries/model_server/model_server"

if ! type "$EXEC" >/dev/null 2>&1; then
  echo "$EXEC not found" >&2
  exit 1
fi

echo -n "Starting model_server... "
"$EXEC" "$@" >/dev/null 2>&1 &
MODEL_SERVER_PID="$!"
kill -0 "$MODEL_SERVER_PID"  # Check that process runs
echo "pid=$MODEL_SERVER_PID"

function stop_model_server() {
  echo -n "Killing model_server with pid=$MODEL_SERVER_PID... "
  kill -9 $MODEL_SERVER_PID && echo "OK"
}
atexit stop_model_server
