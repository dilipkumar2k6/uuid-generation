import time
import threading
from lib.network_util import get_node_id_from_ip

class InstaSnowflake:
    def __init__(self):
        self.EPOCH = 1767225600000 # Jan 1, 2026
        self.SHARD_ID_BITS = 13
        self.SEQUENCE_BITS = 10
        self.MAX_SHARD_ID = (1 << self.SHARD_ID_BITS) - 1
        self.MAX_SEQUENCE = (1 << self.SEQUENCE_BITS) - 1
        self.SHARD_ID_SHIFT = self.SEQUENCE_BITS
        self.TIMESTAMP_SHIFT = self.SEQUENCE_BITS + self.SHARD_ID_BITS

        self.shard_id = get_node_id_from_ip() & self.MAX_SHARD_ID
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
                 (self.shard_id << self.SHARD_ID_SHIFT) | \
                 self.sequence

            return str(id)
