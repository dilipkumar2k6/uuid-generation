import mysql.connector
import time
import sys
import threading

class DualBufferGenerator:
    def __init__(self):
        self.connection = None
        self.buffers = [
            {'current_id': 0, 'max_id': 0, 'ready': False},
            {'current_id': 0, 'max_id': 0, 'ready': False}
        ]
        self.active_idx = 0
        self.is_fetching = False
        self.lock = threading.Lock()
        self.fetch_lock = threading.Lock()

        config = {
            'host': 'mysql-dual-buffer',
            'port': 3306,
            'user': 'root',
            'password': 'rootpassword',
            'database': 'uuid_db',
            'autocommit': False
        }

        for i in range(30):
            try:
                self.connection = mysql.connector.connect(**config)
                print("Successfully connected to MySQL.", flush=True)
                break
            except Exception as e:
                print("Waiting for database connection...", flush=True)
                time.sleep(2)

        if not self.connection:
            print("Database connection failed after retries.", file=sys.stderr)
            sys.exit(1)

        self.fetch_next_block(0)
        if not self.buffers[0]['ready']:
            print("Failed to fetch initial ID segment from database", file=sys.stderr)
            sys.exit(1)

    def fetch_next_block(self, buffer_idx):
        with self.fetch_lock:
            if self.is_fetching:
                return
            self.is_fetching = True

        try:
            cursor = self.connection.cursor(dictionary=True)
            cursor.execute("SELECT max_id, step FROM id_generator WHERE biz_tag = 'user' FOR UPDATE")
            row = cursor.fetchone()

            if not row:
                raise Exception("No row found for biz_tag = 'user'")

            max_id = row['max_id']
            step = row['step']
            new_max_id = max_id + step

            cursor.execute("UPDATE id_generator SET max_id = %s WHERE biz_tag = 'user'", (new_max_id,))
            self.connection.commit()
            cursor.close()

            with self.lock:
                self.buffers[buffer_idx]['current_id'] = max_id
                self.buffers[buffer_idx]['max_id'] = new_max_id
                self.buffers[buffer_idx]['ready'] = True

            print(f"Fetched new block for buffer {buffer_idx}: [{max_id}, {new_max_id})", flush=True)
        except Exception as e:
            print(f"Error fetching next block: {e}", file=sys.stderr)
            if self.connection:
                self.connection.rollback()
        finally:
            with self.fetch_lock:
                self.is_fetching = False

    def next_id_string(self):
        with self.lock:
            active_buffer = self.buffers[self.active_idx]

            if active_buffer['current_id'] >= active_buffer['max_id']:
                standby_idx = 1 - self.active_idx
                standby_buffer = self.buffers[standby_idx]

                while not standby_buffer['ready']:
                    self.lock.release()
                    time.sleep(0.01)
                    self.lock.acquire()

                self.active_idx = standby_idx
                active_buffer['ready'] = False
                active_buffer = standby_buffer

            id = active_buffer['current_id']
            active_buffer['current_id'] += 1

            threshold = active_buffer['max_id'] - int((active_buffer['max_id'] - id) * 0.9)
            if active_buffer['current_id'] == threshold:
                standby_idx = 1 - self.active_idx
                threading.Thread(target=self.fetch_next_block, args=(standby_idx,)).start()

            return str(id)
