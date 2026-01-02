declare module "embedded:io/socket/tcp/tls" {
	import TCP, { TCPOptions } from "embedded:io/socket/tcp";
	import UDP from "embedded:io/socket/udp";

	export type SSLSessionOptions = {
		protocolVersion?: number;
		serverName?: string;
		applicationLayerProtocolNegotiation?: string;
		trace?: number;
		cache?: boolean;
		tls_server_name?: string;
		client_auth?: {
			cipherSuites: string[];
			subjectDN: string;
		};
	};

	export type TLSOptions = TCPOptions & {
		host: string;
		secure: SSLSessionOptions;
	};
	export type TLSDevice = TCPOptions & { io: typeof UDP };

	export default class TLSSocket extends TCP {
		constructor(options: TLSOptions);
	}
}
