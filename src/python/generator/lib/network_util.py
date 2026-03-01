import socket
import psutil

def get_node_id_from_ip():
    try:
        interfaces = psutil.net_if_addrs()
        for interface_name, interface_addresses in interfaces.items():
            if interface_name == 'lo':
                continue
            for address in interface_addresses:
                if address.family == socket.AF_INET:
                    ip = address.address
                    parts = ip.split('.')
                    if len(parts) == 4:
                        part3 = int(parts[2])
                        part4 = int(parts[3])
                        # Take the last 2 bits of part3 and all 8 bits of part4
                        return ((part3 & 0x03) << 8) | part4
    except Exception:
        pass
    return 0
