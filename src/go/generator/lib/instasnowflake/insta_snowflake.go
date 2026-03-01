package instasnowflake

import (
	"fmt"
	"strconv"
	"sync/atomic"
	"time"

	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib"
)

const (
	epoch          = uint64(1767225600000) // Jan 1, 2026
	shardIdBits    = 13
	sequenceBits   = 10
	maxShardId     = (1 << shardIdBits) - 1
	maxSequence    = (1 << sequenceBits) - 1
	shardIdShift   = sequenceBits
	timestampShift = sequenceBits + shardIdBits
)

type InstaSnowflake struct {
	shardId       uint64
	sequence      atomic.Uint64
	lastTimestamp atomic.Uint64
}

func New() *InstaSnowflake {
	return &InstaSnowflake{
		shardId: lib.GetNodeIdFromIp() & maxShardId,
	}
}

func (s *InstaSnowflake) currentTimeMillis() uint64 {
	return uint64(time.Now().UnixMilli())
}

func (s *InstaSnowflake) waitForNextMillis(lastTs uint64) uint64 {
	timestamp := s.currentTimeMillis()
	for timestamp <= lastTs {
		timestamp = s.currentTimeMillis()
	}
	return timestamp
}

func (s *InstaSnowflake) NextId() uint64 {
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
		(s.shardId << shardIdShift) |
		s.sequence.Load()

	return id
}

func (s *InstaSnowflake) NextIdString() string {
	return strconv.FormatUint(s.NextId(), 10)
}
