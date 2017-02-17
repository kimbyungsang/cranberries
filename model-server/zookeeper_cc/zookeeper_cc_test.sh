#!/bin/bash

set -u
set -e
set -o pipefail

source zookeeper_cc/run_zookeeper_server.sh

zookeeper_cc/zookeeper_cc_test_impl "$@"
