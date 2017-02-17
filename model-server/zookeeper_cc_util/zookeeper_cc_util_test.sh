#!/bin/bash

set -u
set -e
set -o pipefail

source zookeeper_cc_util/run_zookeeper_server.sh

zookeeper_cc_util/zookeeper_cc_util_test_impl "$@"
