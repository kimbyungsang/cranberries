import os
import time
import pytest
import kazoo.exceptions
import kazoo.client

@pytest.fixture
def models_base():
    return os.environ["MODELS_TEST_BASE"]


@pytest.yield_fixture
def zk():
    hosts = os.environ["ZOOKEEPER_TEST_HOSTS"]
    test_base = os.environ["ZOOKEEPER_TEST_BASE"]
    zk = kazoo.client.KazooClient(hosts=hosts)
    zk.start()
    zk.ensure_path(test_base)
    zk.stop()

    zk = kazoo.client.KazooClient(hosts="{}/{}".format(hosts, test_base))
    zk.start()
    yield zk
    zk.stop()


def wait_until(cond, msg, timeout_sec=2, delay_sec=0.1):
    end_at = time.time() + timeout_sec
    while time.time() < end_at:
        if cond():
            return True
        time.sleep(delay_sec)
    raise AssertionError(msg)


def try_get_node_data(zk, path):
    try:
        data, stat = zk.get(path)
        return data
    except kazoo.exceptions.NoNodeError:
        return None
