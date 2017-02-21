#!/bin/bash

set -u -e
echo -n "Atext test: "
ATEXIT_RESULT=$(
  bash <<EOF
set -u -e
source sh_utils/sh_utils.sh
atexit echo -n foo 1
atexit echo -n bar 1
atexit echo -n baz 1
EOF
)
ATEXIT_EXPECTED="baz 1bar 1foo 1"
if [[ "$ATEXIT_RESULT" == "$ATEXIT_EXPECTED" ]]; then
  echo OK
else
  echo FAIL
  echo "Expected: $ATEXIT_EXPECTED"
  echo "Got: $ATEXIT_RESULT"
  exit 1
fi
