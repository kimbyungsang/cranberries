#!/bin/bash

set -u -e
export MODELS_TEST_BASE=$(mktemp -d)
tar xf integration_tests/test_models/test_models.tar -C "$MODELS_TEST_BASE"

source zookeeper_cc/run_zookeeper_server.sh
export ZOOKEEPER_TEST_BASE="/cranberries/servers/test"

source integration_tests/run_model_server.sh --zookeeper_base="$ZOOKEEPER_TEST_BASE" --zookeeper_hosts="$ZOOKEEPER_TEST_HOSTS"
