package etcdsnowflake

import (
	"context"
	"fmt"
	"log"
	"os"
	"strconv"
	"sync/atomic"
	"time"

	clientv3 "go.etcd.io/etcd/client/v3"
)

const (
	epoch          = uint64(1767225600000) // Jan 1, 2026
	nodeIdBits     = 10
	sequenceBits   = 12
	maxNodeId      = (1 << nodeIdBits) - 1
	maxSequence    = (1 << sequenceBits) - 1
	nodeIdShift    = sequenceBits
	timestampShift = sequenceBits + nodeIdBits
)

type EtcdSnowflake struct {
	nodeId        uint64
	sequence      atomic.Uint64
	lastTimestamp atomic.Uint64
}

func New() *EtcdSnowflake {
	cli, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{"etcd:2379"},
		DialTimeout: 5 * time.Second,
	})
	if err != nil {
		log.Fatalf("Failed to connect to etcd: %v", err)
	}
	defer cli.Close()

	podName := os.Getenv("HOSTNAME")
	if podName == "" {
		podName = "unknown-pod"
	}

	nodeIdKey := fmt.Sprintf("/snowflake/nodes/%s", podName)

	// Try to get existing node ID
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	resp, err := cli.Get(ctx, nodeIdKey)
	cancel()
	if err != nil {
		log.Fatalf("Failed to get node ID from etcd: %v", err)
	}

	var nodeId uint64
	if len(resp.Kvs) > 0 {
		nodeId, _ = strconv.ParseUint(string(resp.Kvs[0].Value), 10, 64)
		fmt.Printf("Recovered Node ID %d for pod %s\n", nodeId, podName)
	} else {
		// Assign new node ID
		ctx, cancel = context.WithTimeout(context.Background(), 5*time.Second)
		resp, err := cli.Get(ctx, "/snowflake/next_node_id")
		cancel()
		if err != nil {
			log.Fatalf("Failed to get next node ID from etcd: %v", err)
		}

		if len(resp.Kvs) > 0 {
			nodeId, _ = strconv.ParseUint(string(resp.Kvs[0].Value), 10, 64)
		} else {
			nodeId = 0
		}

		if nodeId > maxNodeId {
			log.Fatalf("Node ID exhausted (max %d)", maxNodeId)
		}

		// Save new node ID
		ctx, cancel = context.WithTimeout(context.Background(), 5*time.Second)
		_, err = cli.Put(ctx, nodeIdKey, strconv.FormatUint(nodeId, 10))
		cancel()
		if err != nil {
			log.Fatalf("Failed to save node ID to etcd: %v", err)
		}

		// Increment next node ID
		ctx, cancel = context.WithTimeout(context.Background(), 5*time.Second)
		_, err = cli.Put(ctx, "/snowflake/next_node_id", strconv.FormatUint(nodeId+1, 10))
		cancel()
		if err != nil {
			log.Fatalf("Failed to increment next node ID in etcd: %v", err)
		}

		fmt.Printf("Assigned new Node ID %d for pod %s\n", nodeId, podName)
	}

	return &EtcdSnowflake{
		nodeId: nodeId,
	}
}

func (s *EtcdSnowflake) currentTimeMillis() uint64 {
	return uint64(time.Now().UnixMilli())
}

func (s *EtcdSnowflake) waitForNextMillis(lastTs uint64) uint64 {
	timestamp := s.currentTimeMillis()
	for timestamp <= lastTs {
		timestamp = s.currentTimeMillis()
	}
	return timestamp
}

func (s *EtcdSnowflake) NextId() uint64 {
	timestamp := s.currentTimeMillis()
	lastTs := s.lastTimestamp.Load()

	if timestamp < lastTs {
		fmt.Println("Clock moved backwards. Refusing to generate id.")
		return 0
	}

	if timestamp == lastTs {
		seq := (s.sequence.Add(1)) & maxSequence
		if seq == 0 {
			timestamp = s.waitForNextMillis(lastTs)
		}
	} else {
		s.sequence.Store(0)
	}

	s.lastTimestamp.Store(timestamp)

	id := ((timestamp - epoch) << timestampShift) |
		(s.nodeId << nodeIdShift) |
		s.sequence.Load()

	return id
}

func (s *EtcdSnowflake) NextIdString() string {
	return strconv.FormatUint(s.NextId(), 10)
}
