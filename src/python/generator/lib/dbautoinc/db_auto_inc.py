import mysql.connector
import time
import sys

class DbAutoIncGenerator:
    def __init__(self):
        self.connection = None
        
        config = {
            'host': 'proxysql',
            'port': 6033,
            'user': 'root',
            'password': 'rootpassword',
            'database': 'uuid_db',
            'autocommit': True
        }

        for i in range(30):
            try:
                self.connection = mysql.connector.connect(**config)
                print("Successfully connected to ProxySQL.", flush=True)
                break
            except Exception as e:
                print("Waiting for database connection...", flush=True)
                time.sleep(2)

        if not self.connection:
            print("Database connection failed after retries.", file=sys.stderr)
            sys.exit(1)

    def next_id_string(self):
        if not self.connection:
            print("Database connection not initialized.", file=sys.stderr)
            return ""

        try:
            cursor = self.connection.cursor()
            cursor.execute("REPLACE INTO tickets64 (stub) VALUES ('a')")
            insert_id = cursor.lastrowid
            cursor.close()
            return str(insert_id)
        except Exception as e:
            print(f"Error executing query: {e}", file=sys.stderr)
            return ""
