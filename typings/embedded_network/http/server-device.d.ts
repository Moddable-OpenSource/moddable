import type { DNSUDPDevice } from "embedded:network/dns/resolver/udp";
import type { TCPDevice } from 'embedded:io/socket/tcp';
import HTTPServer from "embedded:network/http/server";


declare global {
	interface NetworkExtensions {
		http: {
			io: typeof HTTPServer;
			dns: DNSUDPDevice;
			socket: TCPDevice
		};
	}

    interface Device {
        network: NetworkExtensions;
    }
}

