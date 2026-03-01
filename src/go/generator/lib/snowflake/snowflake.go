package snowflake

import (
	"fmt"
	"strconv"
	"sync/atomic"
	"time"

	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib"
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

type Snowflake struct {
	nodeId        uint64
	sequence      atomic.Uint64
	lastTimestamp atomic.Uint64
}

func New() *Snowflake {
	return &Snowflake{
		nodeId: lib.GetNodeIdFromIp() & maxNodeId,
	}
}

func (s *Snowflake) currentTimeMillis() uint64 {
	return uint64(time.Now().UnixMilli())
}

func (s *Snowflake) waitForNextMillis(lastTs uint64) uint64 {
	timestamp := s.currentTimeMillis()
	for timestamp <= lastTs {
		timestamp = s.currentTimeMillis()
	}
	return timestamp
}

func (s *Snowflake) NextId() uint64 {
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

func (s *Snowflake) NextIdString() string {
	return strconv.FormatUint(s.NextId(), 10)
}
