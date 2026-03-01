import time
import threading
from lib.network_util import get_node_id_from_ip

class HlcSnowflake:
    def __init__(self):
        self.EPOCH = 1767225600000 # Jan 1, 2026
        self.NODE_ID_BITS = 10
        self.SEQUENCE_BITS = 12
        self.MAX_NODE_ID = (1 << self.NODE_ID_BITS) - 1
        self.MAX_SEQUENCE = (1 << self.SEQUENCE_BITS) - 1
        self.NODE_ID_SHIFT = self.SEQUENCE_BITS
        self.TIMESTAMP_SHIFT = self.SEQUENCE_BITS + self.NODE_ID_BITS

        self.node_id = get_node_id_from_ip() & self.MAX_NODE_ID
        self.logical_time = 0
        self.sequence = 0
        self.lock = threading.Lock()

    def physical_time_millis(self):
        return int(time.time() * 1000)

    def next_id_string(self):
        with self.lock:
            physical_time = self.physical_time_millis()
            current_logical_time = self.logical_time

            if physical_time > current_logical_time:
                new_logical_time = physical_time
                self.logical_time = new_logical_time
                self.sequence = 0
            else:
                new_logical_time = current_logical_time
                self.sequence = (self.sequence + 1) & self.MAX_SEQUENCE
                if self.sequence == 0:
                    new_logical_time = self.logical_time + 1
                    self.logical_time = new_logical_time

            id = ((new_logical_time - self.EPOCH) << self.TIMESTAMP_SHIFT) | \
                 (self.node_id << self.NODE_ID_SHIFT) | \
                 self.sequence

            return str(id)
