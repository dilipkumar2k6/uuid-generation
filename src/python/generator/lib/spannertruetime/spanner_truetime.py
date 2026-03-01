import urllib.request
import urllib.error
import json
import time
import sys
import threading

class SpannerTrueTimeGenerator:
    def __init__(self):
        self.session_name = None
        self.lock = threading.Lock()

        for i in range(30):
            try:
                req = urllib.request.Request('http://spanner:9020/', method='GET')
                with urllib.request.urlopen(req) as response:
                    if response.status in (200, 404):
                        break
            except urllib.error.HTTPError as e:
                if e.code == 404:
                    break
            except Exception as e:
                print("Waiting for Spanner TrueTime emulator...", flush=True)
                time.sleep(2)

        try:
            db_name = "projects/test-project/instances/test-instance/databases/test-db"
            url = f"http://spanner:9020/v1/{db_name}/sessions"
            data = json.dumps({}).encode('utf-8')
            req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'}, method='POST')
            
            with urllib.request.urlopen(req) as response:
                res_data = json.loads(response.read().decode('utf-8'))
                if 'name' in res_data:
                    self.session_name = res_data['name']
                    print(f"Successfully created Spanner TrueTime session: {self.session_name}", flush=True)
                else:
                    print(f"Session name not found in response: {res_data}", file=sys.stderr)
                    sys.exit(1)
        except Exception as e:
            print(f"Failed to initialize Spanner TrueTime session: {e}", file=sys.stderr)
            sys.exit(1)

    def next_id_string(self):
        if not self.session_name:
            print("Spanner TrueTime session not initialized.", file=sys.stderr)
            return ""

        # Use a lock to ensure only one thread uses the session at a time
        # The emulator might not support concurrent transactions on the same session well
        with self.lock:
            try:
                # 1. Begin Transaction
                begin_url = f"http://spanner:9020/v1/{self.session_name}:beginTransaction"
                begin_data = json.dumps({"options": {"readWrite": {}}}).encode('utf-8')
                begin_req = urllib.request.Request(begin_url, data=begin_data, headers={'Content-Type': 'application/json'}, method='POST')
                
                with urllib.request.urlopen(begin_req) as response:
                    begin_res = json.loads(response.read().decode('utf-8'))
                    transaction_id = begin_res.get('id')
                    
                if not transaction_id:
                    print("Failed to begin transaction", file=sys.stderr)
                    return ""

                # 2. Execute SQL
                url = f"http://spanner:9020/v1/{self.session_name}:executeSql"
                data = json.dumps({
                    "sql": "INSERT INTO events (description) VALUES ('event') THEN RETURN id",
                    "transaction": {"id": transaction_id}
                }).encode('utf-8')
                req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'}, method='POST')
                
                generated_id = ""
                with urllib.request.urlopen(req) as response:
                    res_data = json.loads(response.read().decode('utf-8'))
                    if 'rows' in res_data and len(res_data['rows']) > 0 and len(res_data['rows'][0]) > 0:
                        generated_id = str(res_data['rows'][0][0])
                    else:
                        print(f"Failed to extract ID from response: {res_data}", file=sys.stderr)

                # 3. Commit Transaction
                commit_url = f"http://spanner:9020/v1/{self.session_name}:commit"
                commit_data = json.dumps({
                    "transactionId": transaction_id,
                    "mutations": []
                }).encode('utf-8')
                commit_req = urllib.request.Request(commit_url, data=commit_data, headers={'Content-Type': 'application/json'}, method='POST')
                urllib.request.urlopen(commit_req)

                return generated_id
            except urllib.error.HTTPError as e:
                error_body = e.read().decode('utf-8')
                print(f"Error executing SQL: {e}. Body: {error_body}", file=sys.stderr)
                return ""
            except Exception as e:
                print(f"Error executing SQL: {e}", file=sys.stderr)
                return ""
