package sonyflake

import (
	"fmt"
	"strconv"
	"sync/atomic"
	"time"

	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib"
)

const (
	epoch          = uint64(1767225600000) // Jan 1, 2026
	machineIdBits  = 16
	sequenceBits   = 8
	maxMachineId   = (1 << machineIdBits) - 1
	maxSequence    = (1 << sequenceBits) - 1
	machineIdShift = sequenceBits
	timestampShift = sequenceBits + machineIdBits
)

type Sonyflake struct {
	machineId     uint64
	sequence      atomic.Uint64
	lastTimestamp atomic.Uint64
}

func New() *Sonyflake {
	return &Sonyflake{
		machineId: lib.GetNodeIdFromIp() & maxMachineId,
	}
}

func (s *Sonyflake) currentTime10ms() uint64 {
	return uint64(time.Now().UnixMilli() / 10)
}

func (s *Sonyflake) waitForNext10ms(lastTs uint64) uint64 {
	timestamp := s.currentTime10ms()
	for timestamp <= lastTs {
		timestamp = s.currentTime10ms()
	}
	return timestamp
}

func (s *Sonyflake) NextId() uint64 {
	timestamp := s.currentTime10ms()
	lastTs := s.lastTimestamp.Load()

	if timestamp < lastTs {
		fmt.Println("Clock moved backwards. Refusing to generate id.")
		return 0
	}

	if timestamp == lastTs {
		seq := (s.sequence.Add(1)) & maxSequence
		if seq == 0 {
			timestamp = s.waitForNext10ms(lastTs)
		}
	} else {
		s.sequence.Store(0)
	}

	s.lastTimestamp.Store(timestamp)

	id := ((timestamp - (epoch / 10)) << timestampShift) |
		(s.machineId << machineIdShift) |
		s.sequence.Load()

	return id
}

func (s *Sonyflake) NextIdString() string {
	return strconv.FormatUint(s.NextId(), 10)
}
