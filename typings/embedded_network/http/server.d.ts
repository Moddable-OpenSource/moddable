declare module "embedded:network/http/server" {
    import type Listener from "embedded:io/socket/listener";
    import type TCP from "embedded:io/socket/tcp";
    import "embedded:network/http/server-device";

    export interface HTTPServerOptions {
        io: typeof Listener;
        port?: number;
        onConnect?: (this: HTTPServer, connection: HTTPServerConnection) => void;
    }

    export interface HTTPServerRoute {
        onRequest?: (this: HTTPServerConnection, request: { method: string, path: string, headers: Map<string, string | string[]>}) => void;
        onReadable?: (this: HTTPServerConnection, count: number) => void;
        onResponse?: (this: HTTPServerConnection, response: HTTPServerResponseOptions) => void;
        onWritable?: (this: HTTPServerConnection, count: number) => void;
        onDone?: (this: HTTPServerConnection, ) => void;
        onError?: (this: HTTPServerConnection, error?: string) => void;
    }

    export interface HTTPServerResponseOptions {
        status: number;
        headers: Map<string, string | string[]>;
    }

    export interface HTTPServerConnection {
        close(): void;
        detach(): TCP;
        accept(options: HTTPServerRoute): void;
        respond(options: HTTPServerResponseOptions): void;
        read(count: number): ArrayBuffer;
        write(payload?: ArrayBuffer): number;
        route: HTTPServerRoute;
    }
  
    export default class HTTPServer {
      constructor(options: HTTPServerOptions);
      close(): void;
	  get port(): number;
    }
  }
  