#!/bin/bash
import os.path
import sys
from utils import *


def test_model_loading(zk, models_base):
    zk.ensure_path("/aspired-models/a")
    zk.create(
        "/aspired-models/a/1",
        os.path.join(models_base, "models", "a", "1")
    )
    def model_available():
        return try_get_node_data(zk, "/current-models/a/1") == "kAvailable"
    assert wait_until(model_available, "Model did not become available")


if __name__ == "__main__":
    run_tests()
