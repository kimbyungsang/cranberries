#!/bin/bash

if ! [ -z "${SH_UTILS_SOURCED+x}" ]; then
  return
fi
SH_UTILS_SOURCED=1

# Courtesy of https://github.com/pagespeed/mod_pagespeed/blob/d55c2d7a2c64fab183f77571a18dad80ffe3cb1b/install/shell_utils.sh
# Usage: wait_cmd_with_timeout TIMEOUT_SECS CMD [ARG ...]
# Waits until CMD succeed or TIMEOUT_SECS passes, printing progress dots.
# Returns exit code of CMD. It works with CMD which does not terminate
# immediately.
function wait_cmd_with_timeout() {
  # Bash magic variable which is increased every second. Note that assignment
  # does not reset timer, only counter, i.e. it's possible that it will become 1
  # earlier than after 1s.
  SECONDS=0
  while [[ "$SECONDS" -le "$1" ]]; do  # -le because of measurement error.
    if eval "${@:2}"; then
      return 0
    fi
    sleep 0.1
    echo -n .
  done
  eval "${@:2}"
}

# We have to set at least one item, otherwise the array would be considered
# unset.
ATEXIT=("true")
function atexit() {
  ATEXIT=("$*" "${ATEXIT[@]}")
}

if [[ -n "$(trap -p EXIT)" ]]; then
  echo "EXIT trap is already set up, failing" >&2
  exit 1
fi

function atexit_handler() {
  local EXPR
  for EXPR in "${ATEXIT[@]}"; do
    eval "$EXPR"
  done
}

trap atexit_handler EXIT
