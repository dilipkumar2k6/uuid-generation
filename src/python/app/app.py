import socket
import time
import threading

def request_uuid(thread_id):
    while True:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect(('127.0.0.1', 8080))
                data = s.recv(1024)
                if data:
                    print(f"[Thread {thread_id}] Received UUID: {data.decode('utf-8')}", flush=True)
                else:
                    print(f"[Thread {thread_id}] Failed to read UUID", flush=True)
        except Exception as e:
            print(f"[Thread {thread_id}] Connection Failed. Retrying...", flush=True)
        
        time.sleep(0.5)

if __name__ == "__main__":
    print("App container starting with 5 concurrent threads...", flush=True)
    
    num_threads = 5
    threads = []
    
    for i in range(1, num_threads + 1):
        t = threading.Thread(target=request_uuid, args=(i,))
        t.daemon = True
        t.start()
        threads.append(t)
        
    for t in threads:
        t.join()
