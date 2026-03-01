import urllib.request
import urllib.error
import json
import time
import sys

class SpannerGenerator:
    def __init__(self):
        self.session_name = None

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
                print("Waiting for Spanner emulator...", flush=True)
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
                    print(f"Successfully created Spanner session: {self.session_name}", flush=True)
                else:
                    print(f"Session name not found in response: {res_data}", file=sys.stderr)
                    sys.exit(1)
        except Exception as e:
            print(f"Failed to initialize Spanner session: {e}", file=sys.stderr)
            sys.exit(1)

    def next_id_string(self):
        if not self.session_name:
            print("Spanner session not initialized.", file=sys.stderr)
            return ""

        try:
            url = f"http://spanner:9020/v1/{self.session_name}:executeSql"
            data = json.dumps({"sql": "SELECT GET_NEXT_SEQUENCE_VALUE(SEQUENCE global_id_seq)"}).encode('utf-8')
            req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'}, method='POST')
            
            with urllib.request.urlopen(req) as response:
                res_data = json.loads(response.read().decode('utf-8'))
                if 'rows' in res_data and len(res_data['rows']) > 0 and len(res_data['rows'][0]) > 0:
                    return str(res_data['rows'][0][0])
            print(f"Failed to extract sequence value from response: {res_data}", file=sys.stderr)
            return ""
        except Exception as e:
            print(f"Error executing SQL: {e}", file=sys.stderr)
            return ""
