#!/bin/bash
import time
import os
import os.path
import sys
import kazoo.exceptions
from kazoo.client import KazooClient


def main():
    zk = KazooClient(hosts=os.environ["ZOOKEEPER_TEST_HOSTS"])
    zk_base = os.environ["ZOOKEEPER_TEST_BASE"]
    models_base = os.environ["MODELS_TEST_BASE"]

    zk.start()
    try:
        zk.ensure_path("{}/aspired-models/a".format(zk_base))
        zk.create(
            "{}/aspired-models/a/1".format(zk_base),
            os.path.join(models_base, "models", "a", "1")
        )
        zk_current_model_path = "{}/current-models/a/1".format(zk_base)

        started_at = time.time()
        while True:
            time.sleep(0.1)
            if (time.time() - started_at) >= 2:
                print("Timed out")
                sys.exit(1)
            try:
                data, stat = zk.get(zk_current_model_path)
            except kazoo.exceptions.NoNodeError:
                continue
            if data == "kAvailable":
                print("Model became available")
                break
    finally:
        zk.stop()

if __name__ == "__main__":
    main()
