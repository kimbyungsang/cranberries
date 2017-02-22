#!/bin/bash

set -u -e
source sh_utils/sh_utils.sh

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
