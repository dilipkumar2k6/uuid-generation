#!/bin/bash

set -e

CLUSTER_NAME="uuid-test-cluster"

echo "========================================"
echo "  Billion-Scale UUID Generator Setup  "
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
echo "[+] Deploying to Kubernetes..."
kubectl apply -f deployment.yaml

# 7. Wait for Pod to be ready
echo "[+] Waiting for Pod to be ready..."
kubectl wait --for=condition=ready pod -l app=uuid-generator --timeout=60s

# 8. Show logs
echo "========================================"
echo "  Deployment Successful! Showing Logs:  "
echo "========================================"
POD_NAME=$(kubectl get pods -l app=uuid-generator -o jsonpath="{.items[0].metadata.name}")
kubectl logs -f "$POD_NAME" -c app
