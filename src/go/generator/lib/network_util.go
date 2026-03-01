package lib

import (
	"net"
)

func GetNodeIdFromIp() uint64 {
	interfaces, err := net.Interfaces()
	if err != nil {
		return 0
	}

	for _, i := range interfaces {
		addrs, err := i.Addrs()
		if err != nil {
			continue
		}
		for _, addr := range addrs {
			var ip net.IP
			switch v := addr.(type) {
			case *net.IPNet:
				ip = v.IP
			case *net.IPAddr:
				ip = v.IP
			}

			if ip == nil || ip.IsLoopback() {
				continue
			}

			ip = ip.To4()
			if ip == nil {
				continue // not an ipv4 address
			}

			// ip is a 4-byte slice. We want the last 10 bits.
			// The last two bytes are ip[2] and ip[3].
			// We take the last 2 bits of ip[2] and all 8 bits of ip[3].
			return uint64(ip[2]&0x03)<<8 | uint64(ip[3])
		}
	}
	return 0
}
