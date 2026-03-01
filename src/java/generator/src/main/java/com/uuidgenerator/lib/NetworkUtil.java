package com.uuidgenerator.lib;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Enumeration;

public class NetworkUtil {
    public static long getNodeIdFromIp() {
        try {
            Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
            while (interfaces.hasMoreElements()) {
                NetworkInterface iface = interfaces.nextElement();
                if (iface.isLoopback() || !iface.isUp()) {
                    continue;
                }

                Enumeration<InetAddress> addresses = iface.getInetAddresses();
                while (addresses.hasMoreElements()) {
                    InetAddress addr = addresses.nextElement();
                    if (addr instanceof Inet4Address) {
                        byte[] ip = addr.getAddress();
                        // ip is a 4-byte array. We want the last 10 bits.
                        // The last two bytes are ip[2] and ip[3].
                        // We take the last 2 bits of ip[2] and all 8 bits of ip[3].
                        long part1 = (ip[2] & 0x03) << 8;
                        long part2 = ip[3] & 0xFF;
                        return part1 | part2;
                    }
                }
            }
        } catch (Exception e) {
            // Ignore and return 0
        }
        return 0;
    }
}
