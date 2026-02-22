# Billion-Scale UUID Generator

This project implements a high-performance, billion-scale UUID generator using a Snowflake sidecar pattern, as described in the architectural recommendation for a billion-scale system at https://dilipkumar.medium.com/design-a-system-to-generate-a-unique-id-1517dc624975.

## Architecture

```text
+---------------------------------------------------------------------------------+
|                              Kubernetes Pod                                     |
|                                                                                 |
|  +-------------------------+                       +-------------------------+  |
|  |                         |                       |                         |  |
|  |      App Container      |                       |    Snowflake Sidecar    |  |
|  |      (app.cpp)          |                       |    (snowflake.cpp)      |  |
|  |                         |                       |                         |  |
|  |   [Consumer]            |                       |   [Generator]           |  |
|  |   - Spawns 5 threads    | <--- Localhost IPC --->   - Listens on port 8080|  |
|  |   - Requests UUIDs      |      (TCP port 8080)  |   - Derives Node ID     |  |
|  |   - Prints to stdout    |                       |   - Generates 64-bit ID |  |
|  |                         |                       |   - Flips bits (~id)    |  |
|  +-------------------------+                       +-------------------------+  |
|                                                                                 |
+---------------------------------------------------------------------------------+
```

The system consists of two main components deployed together in a single Kubernetes Pod (Sidecar pattern):

1.  **App Container (Consumer)**: A lightweight C++ microservice (`app.cpp`) that acts as the main application. It connects to the Snowflake sidecar via localhost IPC (TCP port 8080) to request unique IDs.
2.  **Snowflake Sidecar (Generator)**: A high-performance C++ binary (`snowflake.cpp`) that generates 64-bit K-sortable IDs based on the Snowflake algorithm. It uses:
    -   **Time**: 41-bit timestamp derived from the system clock (milliseconds since custom epoch).
    -   **Node**: 10-bit node ID dynamically derived from the last 10 bits of the container's IPv4 address.
    -   **Seq**: 12-bit local atomic counter to handle multiple requests within the same millisecond.

### Bit Reversal Technique
As per specific requirements, the generated 64-bit UUID has its bits flipped (`~id`) before being returned to the App container. *Note: This bit reversal destroys the K-sortability of the original Snowflake ID, but fulfills the specific design request.*

## Prerequisites

- Docker
- `kubectl` configured to communicate with your cluster
- `kind` (Kubernetes in Docker) for local testing

### Installing `kind` on Linux

To quickly set up a local Kubernetes cluster for testing, you can install `kind`. Run the following commands on your Linux machine:

```bash
curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-amd64
chmod +x ./kind
sudo mv ./kind /usr/local/bin/kind
```

Once installed, create a new local cluster:

```bash
kind create cluster --name uuid-test-cluster
```

## Quick Start (Automated)

The easiest way to build, deploy, and test the application is to use the provided `run.sh` script. This script will automatically:
1. Install `kind` if it's not already installed.
2. Clean up any existing test clusters.
3. Create a new `kind` cluster.
4. Build the Docker images.
5. Load the images into the cluster.
6. Deploy the application.
7. Wait for the Pod to be ready and automatically tail the logs.

Simply run:

```bash
./run.sh
```

## Manual Setup and Testing

If you prefer to run the steps manually, follow the instructions below.

### 1. Build the Docker Images

First, build the Docker images for both the App and the Snowflake sidecar from the root of the project directory:

```bash
# Build the App container image
docker build -t uuid-app -f Dockerfile.app .

# Build the Snowflake sidecar image
docker build -t uuid-snowflake -f Dockerfile.snowflake .
```

### 2. Load Images into `kind` Cluster

Since `kind` runs its own Docker daemon inside its nodes, you need to load the locally built images into the cluster so Kubernetes can find them:

```bash
kind load docker-image uuid-app:latest --name uuid-test-cluster
kind load docker-image uuid-snowflake:latest --name uuid-test-cluster
```

### 3. Deploy to Kubernetes

Apply the provided Kubernetes deployment manifest. This will create a single Pod containing both the App and Snowflake containers.

```bash
kubectl apply -f deployment.yaml
```

### 3. Verify the Deployment

Check that the Pod is running successfully:

```bash
kubectl get pods -l app=uuid-generator
```

You should see a Pod named `uuid-generator-<hash>` with `2/2` containers ready and a status of `Running`.

### 4. View the Generated UUIDs

The App container continuously requests a new UUID every second and prints it to standard output. You can view these generated UUIDs by checking the logs of the `app` container within the deployed Pod:

```bash
# Get the exact pod name
POD_NAME=$(kubectl get pods -l app=uuid-generator -o jsonpath="{.items[0].metadata.name}")

# Tail the logs of the 'app' container
kubectl logs -f $POD_NAME -c app
```

You should see output similar to this, showing the bit-reversed 64-bit UUIDs:

```
App container starting...
Received UUID: 18446744073709551615
Received UUID: 18446744073709551614
...
```

### 6. Cleanup

To remove the deployment from your Kubernetes cluster:

```bash
kubectl delete -f deployment.yaml
```

If you want to completely delete the `kind` cluster:

```bash
kind delete cluster --name uuid-test-cluster
```
