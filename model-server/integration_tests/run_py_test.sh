#!/bin/bash

set -u -e
source integration_tests/run_all_servers.sh
python -m pytest integration_tests/$(basename "$0")_impl.py
