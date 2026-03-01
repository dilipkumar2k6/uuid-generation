import time
import threading
from lib.network_util import get_node_id_from_ip

class Sonyflake:
    def __init__(self):
        self.EPOCH = 1767225600000 # Jan 1, 2026
        self.MACHINE_ID_BITS = 16
        self.SEQUENCE_BITS = 8
        self.MAX_MACHINE_ID = (1 << self.MACHINE_ID_BITS) - 1
        self.MAX_SEQUENCE = (1 << self.SEQUENCE_BITS) - 1
        self.MACHINE_ID_SHIFT = self.SEQUENCE_BITS
        self.TIMESTAMP_SHIFT = self.SEQUENCE_BITS + self.MACHINE_ID_BITS

        self.machine_id = get_node_id_from_ip() & self.MAX_MACHINE_ID
        self.sequence = 0
        self.last_timestamp = -1
        self.lock = threading.Lock()

    def current_time_10ms(self):
        return int(time.time() * 100)

    def wait_for_next_10ms(self, last_ts):
        timestamp = self.current_time_10ms()
        while timestamp <= last_ts:
            timestamp = self.current_time_10ms()
        return timestamp

    def next_id_string(self):
        with self.lock:
            timestamp = self.current_time_10ms()

            if timestamp < self.last_timestamp:
                print("Clock moved backwards. Refusing to generate id.")
                return "0"

            if timestamp == self.last_timestamp:
                self.sequence = (self.sequence + 1) & self.MAX_SEQUENCE
                if self.sequence == 0:
                    timestamp = self.wait_for_next_10ms(self.last_timestamp)
            else:
                self.sequence = 0

            self.last_timestamp = timestamp

            id = ((timestamp - (self.EPOCH // 10)) << self.TIMESTAMP_SHIFT) | \
                 (self.machine_id << self.MACHINE_ID_SHIFT) | \
                 self.sequence

            return str(id)
