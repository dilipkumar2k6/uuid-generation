import os
import socket
import threading
import sys

from lib.dbautoinc.db_auto_inc import DbAutoIncGenerator
from lib.dualbuffer.dual_buffer import DualBufferGenerator
from lib.etcdsnowflake.etcd_snowflake import EtcdSnowflake
from lib.hlcsnowflake.hlc_snowflake import HlcSnowflake
from lib.instasnowflake.insta_snowflake import InstaSnowflake
from lib.snowflake.snowflake import Snowflake
from lib.sonyflake.sonyflake import Sonyflake
from lib.spanner.spanner import SpannerGenerator
from lib.spannertruetime.spanner_truetime import SpannerTrueTimeGenerator
from lib.uuidv4.uuidv4 import UuidV4Generator
from lib.uuidv7.uuidv7 import UuidV7Generator

def handle_client(client_socket, generator):
    try:
        uuid_str = generator.next_id_string()
        client_socket.sendall(uuid_str.encode('utf-8'))
    except Exception as e:
        print(f"Error generating ID: {e}", file=sys.stderr)
    finally:
        client_socket.close()

def main():
    # ---------------------------------------------------------
    # 1. Determine Generator Type
    # ---------------------------------------------------------
    gen_type = os.environ.get("GENERATOR_TYPE", "SNOWFLAKE")
    
    if gen_type == "HLC_SNOWFLAKE":
        print("Initializing HLC Snowflake generator...", flush=True)
        generator = HlcSnowflake()
    elif gen_type == "INSTA_SNOWFLAKE":
        print("Initializing Instagram Snowflake generator...", flush=True)
        generator = InstaSnowflake()
    elif gen_type == "SONYFLAKE":
        print("Initializing Sonyflake generator...", flush=True)
        generator = Sonyflake()
    elif gen_type == "UUIDV4":
        print("Initializing UUID Version 4 generator...", flush=True)
        generator = UuidV4Generator()
    elif gen_type == "UUIDV7":
        print("Initializing UUID Version 7 generator...", flush=True)
        generator = UuidV7Generator()
    elif gen_type == "DB_AUTO_INC":
        print("Initializing Database Auto-Increment generator...", flush=True)
        generator = DbAutoIncGenerator()
    elif gen_type == "DUAL_BUFFER":
        print("Initializing Dual Buffer generator...", flush=True)
        generator = DualBufferGenerator()
    elif gen_type == "ETCD_SNOWFLAKE":
        print("Initializing Etcd-Coordinated Snowflake generator...", flush=True)
        generator = EtcdSnowflake()
    elif gen_type == "SPANNER":
        print("Initializing Spanner Sequence generator...", flush=True)
        generator = SpannerGenerator()
    elif gen_type == "SPANNER_TRUETIME":
        print("Initializing Spanner TrueTime generator...", flush=True)
        generator = SpannerTrueTimeGenerator()
    else:
        print("Initializing Standard Snowflake generator...", flush=True)
        generator = Snowflake()

    # ---------------------------------------------------------
    # 2. Setup TCP Server Socket
    # ---------------------------------------------------------
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server.bind(('0.0.0.0', 8080))
        server.listen(5)
        print("Sidecar listening on port 8080...", flush=True)
    except Exception as e:
        print(f"Listen failed: {e}", file=sys.stderr)
        sys.exit(1)

    # ---------------------------------------------------------
    # 3. Main Server Loop
    # ---------------------------------------------------------
    while True:
        try:
            client_socket, addr = server.accept()
            # Handle each connection in a new thread
            client_thread = threading.Thread(target=handle_client, args=(client_socket, generator))
            client_thread.daemon = True
            client_thread.start()
        except Exception as e:
            print(f"Accept failed: {e}", file=sys.stderr)

if __name__ == "__main__":
    main()
