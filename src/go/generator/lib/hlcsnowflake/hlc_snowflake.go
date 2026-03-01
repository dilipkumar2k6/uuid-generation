package hlcsnowflake

import (
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

type HlcSnowflake struct {
	nodeId      uint64
	logicalTime atomic.Uint64
	sequence    atomic.Uint64
}

func New() *HlcSnowflake {
	return &HlcSnowflake{
		nodeId: lib.GetNodeIdFromIp() & maxNodeId,
	}
}

func (s *HlcSnowflake) physicalTimeMillis() uint64 {
	return uint64(time.Now().UnixMilli())
}

func (s *HlcSnowflake) NextId() uint64 {
	physicalTime := s.physicalTimeMillis()
	currentLogicalTime := s.logicalTime.Load()

	var newLogicalTime uint64

	if physicalTime > currentLogicalTime {
		newLogicalTime = physicalTime
		s.logicalTime.Store(newLogicalTime)
		s.sequence.Store(0)
	} else {
		newLogicalTime = currentLogicalTime
		seq := (s.sequence.Add(1)) & maxSequence
		if seq == 0 {
			newLogicalTime = s.logicalTime.Add(1)
		}
	}

	id := ((newLogicalTime - epoch) << timestampShift) |
		(s.nodeId << nodeIdShift) |
		s.sequence.Load()

	return id
}

func (s *HlcSnowflake) NextIdString() string {
	return strconv.FormatUint(s.NextId(), 10)
}
