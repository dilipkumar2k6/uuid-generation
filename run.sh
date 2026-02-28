#!/bin/bash

set -e

CLUSTER_NAME="uuid-test-cluster"

# Default generator algorithm
ID_GENERATOR_ALGO="SNOWFLAKE"

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --id_generator_algo) ID_GENERATOR_ALGO="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

echo "========================================"
echo "  Billion-Scale UUID Generator Setup  "
echo "  Generator Type: $ID_GENERATOR_ALGO"
echo "========================================"

# 1. Install kind if missing
if ! command -v kind &> /dev/null; then
    echo "[+] Installing kind..."
    curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-amd64
    chmod +x ./kind
    sudo mv ./kind /usr/local/bin/kind
else
    echo "[+] kind is already installed."
fi

# 2. Cleanup existing cluster/deployment if they exist
echo "[+] Cleaning up previous deployments..."
if kind get clusters | grep -q "$CLUSTER_NAME"; then
    echo "    Deleting existing kind cluster: $CLUSTER_NAME"
    kind delete cluster --name "$CLUSTER_NAME"
fi

# 3. Create kind cluster
echo "[+] Creating kind cluster: $CLUSTER_NAME..."
kind create cluster --name "$CLUSTER_NAME"

# 4. Build Docker images
echo "[+] Building Docker images..."
docker build -t uuid-app:latest -f Dockerfile.app .
docker build -t uuid-snowflake:latest -f Dockerfile.snowflake .

# 5. Load images into kind cluster
echo "[+] Loading images into kind cluster..."
kind load docker-image uuid-app:latest --name "$CLUSTER_NAME"
kind load docker-image uuid-snowflake:latest --name "$CLUSTER_NAME"

# 6. Deploy to Kubernetes
echo "[+] Deploying to Kubernetes with GENERATOR_TYPE=$ID_GENERATOR_ALGO..."

# If using DB_AUTO_INC, deploy the database tier first
if [ "$ID_GENERATOR_ALGO" = "DB_AUTO_INC" ]; then
    echo "[+] Deploying Database Auto-Increment tier (MySQL + ProxySQL)..."
    kubectl apply -f lib/db-auto-inc/mysql-deployment.yaml
    kubectl apply -f lib/db-auto-inc/proxysql-deployment.yaml
    
    echo "[+] Waiting for database tier to become ready..."
    kubectl wait --for=condition=available --timeout=120s deployment/mysql1
    kubectl wait --for=condition=available --timeout=120s deployment/mysql2
    kubectl wait --for=condition=available --timeout=120s deployment/proxysql
fi

# If using DUAL_BUFFER, deploy the dual-buffer database tier first
if [ "$ID_GENERATOR_ALGO" = "DUAL_BUFFER" ]; then
    echo "[+] Deploying Dual Buffer database tier (MySQL)..."
    kubectl apply -f lib/dual-buffer/mysql-deployment.yaml
    
    echo "[+] Waiting for database tier to become ready..."
    kubectl wait --for=condition=available --timeout=120s deployment/mysql-dual-buffer
fi

# If using ETCD_SNOWFLAKE, deploy the etcd tier first
if [ "$ID_GENERATOR_ALGO" = "ETCD_SNOWFLAKE" ]; then
    echo "[+] Deploying Etcd tier..."
    kubectl apply -f lib/etcd-snowflake/etcd-deployment.yaml
    
    echo "[+] Waiting for etcd to become ready..."
    kubectl wait --for=condition=available --timeout=120s deployment/etcd
fi

# If using SPANNER, deploy the Spanner emulator tier first
if [ "$ID_GENERATOR_ALGO" = "SPANNER" ]; then
    echo "[+] Deploying Spanner Emulator tier..."
    kubectl apply -f lib/spanner/spanner-deployment.yaml
    
    echo "[+] Waiting for Spanner Emulator to become ready..."
    kubectl wait --for=condition=available --timeout=120s deployment/spanner
    
    echo "[+] Waiting for Spanner initialization job to complete..."
    kubectl wait --for=condition=complete --timeout=300s job/spanner-init
fi

# If using SPANNER_TRUETIME, deploy the Spanner TrueTime emulator tier first
if [ "$ID_GENERATOR_ALGO" = "SPANNER_TRUETIME" ]; then
    echo "[+] Deploying Spanner TrueTime Emulator tier..."
    kubectl apply -f lib/spanner-truetime/spanner-truetime-deployment.yaml
    
    echo "[+] Waiting for Spanner Emulator to become ready..."
    kubectl wait --for=condition=available --timeout=120s deployment/spanner-truetime
    
    echo "[+] Waiting for Spanner initialization job to complete..."
    kubectl wait --for=condition=complete --timeout=300s job/spanner-truetime-init
fi

# Create a temporary deployment file with the correct generator type
sed "s/value: \"SNOWFLAKE\"/value: \"$ID_GENERATOR_ALGO\"/g" deployment.yaml > deployment_tmp.yaml
kubectl apply -f deployment_tmp.yaml
rm deployment_tmp.yaml

# 7. Wait for Pod to be ready
echo "[+] Waiting for Pod to be ready..."
kubectl wait --for=condition=ready pod -l app=uuid-generator --timeout=60s

# 8. Show logs
echo "========================================"
echo "  Deployment Successful! Showing Logs:  "
echo "========================================"
POD_NAME=$(kubectl get pods -l app=uuid-generator -o jsonpath="{.items[0].metadata.name}")
kubectl logs -f "$POD_NAME" -c app
