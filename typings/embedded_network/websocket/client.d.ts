declare module "embedded:network/websocket/client" {
	import type { DNSUDPDevice } from "embedded:network/dns/resolver/udp";
	import type { Buffer } from "embedded:io/_common";
	import type TCP from "embedded:io/socket/tcp";
	import type { TCPDevice } from "embedded:io/socket/tcp";
	import type TLSSocket from "embedded:io/socket/tcp/tls";
	import type { TLSDevice } from "embedded:io/socket/tcp/tls";
	import "embedded:network/websocket/client-device";
	
	interface WebSocketClientReadableOptions {
		more: boolean;
		binary: boolean;
	}

	export interface WebSocketClientWriteOptions {
		binary?: boolean;
		more?: boolean;
		opcode?: WebSocketClientOpcode;
	}

	type WebSocketClientOpcode = 1 | 2 | 8 | 9 | 10;

	type WebSocketClientOptions = ((
		{
			attach?: TCP | TLSSocket;
		} | {
			host?: string;
			port?: number;
			socket: TCPDevice | TLSDevice;
		}) & {
			protocol?: string;
			headers?: Map<string, string | string[]>;
			dns?: DNSUDPDevice;
			onReadable?: (this: WebSocketClient, count: number, options: WebSocketClientReadableOptions) => void;
			onWritable?: (this: WebSocketClient, count: number) => void;
			onControl?: (this: WebSocketClient, opcode: WebSocketClientOpcode, buffer: ArrayBuffer) => void;
			onClose?: (this: WebSocketClient) => void;
			onError?: (this: WebSocketClient, error?: string) => void;
		}
	);

	export type WebSocketClientDevice = WebSocketClientOptions & { io: typeof WebSocketClient };

	export default class WebSocketClient {
		constructor(options: WebSocketClientOptions);
		close(): void;
		read(count?: number): ArrayBuffer | undefined;
		write(message: Buffer, options?: WebSocketClientWriteOptions): number;
		get format(): "number" | "buffer";
		set format(value: "number" | "buffer");

		static readonly text: 1;
		static readonly binary: 2;
		static readonly close: 8;
		static readonly ping: 9;
		static readonly pong: 10;
	}
}
