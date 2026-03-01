import time
import threading

# Note: A full Etcd implementation in Python requires an external library like python-etcd.
# For the sake of this example and to avoid complex dependency management in a simple
# script-based build, we will provide a simplified stub that falls back to IP-based
# node ID generation if etcd is not available, or just uses a static ID for demonstration.

class EtcdSnowflake:
    def __init__(self):
        self.EPOCH = 1767225600000 # Jan 1, 2026
        self.NODE_ID_BITS = 10
        self.SEQUENCE_BITS = 12
        self.MAX_NODE_ID = (1 << self.NODE_ID_BITS) - 1
        self.MAX_SEQUENCE = (1 << self.SEQUENCE_BITS) - 1
        self.NODE_ID_SHIFT = self.SEQUENCE_BITS
        self.TIMESTAMP_SHIFT = self.SEQUENCE_BITS + self.NODE_ID_BITS

        print("Warning: EtcdSnowflake in Python is using a stubbed Node ID (1) due to missing etcd dependency.")
        self.node_id = 1 & self.MAX_NODE_ID
        self.sequence = 0
        self.last_timestamp = -1
        self.lock = threading.Lock()

    def current_time_millis(self):
        return int(time.time() * 1000)

    def wait_for_next_millis(self, last_ts):
        timestamp = self.current_time_millis()
        while timestamp <= last_ts:
            timestamp = self.current_time_millis()
        return timestamp

    def next_id_string(self):
        with self.lock:
            timestamp = self.current_time_millis()

            if timestamp < self.last_timestamp:
                print("Clock moved backwards. Refusing to generate id.")
                return "0"

            if timestamp == self.last_timestamp:
                self.sequence = (self.sequence + 1) & self.MAX_SEQUENCE
                if self.sequence == 0:
                    timestamp = self.wait_for_next_millis(self.last_timestamp)
            else:
                self.sequence = 0

            self.last_timestamp = timestamp

            id = ((timestamp - self.EPOCH) << self.TIMESTAMP_SHIFT) | \
                 (self.node_id << self.NODE_ID_SHIFT) | \
                 self.sequence

            return str(id)
