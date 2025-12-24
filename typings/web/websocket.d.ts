declare module "web/websocket" {
  import Headers from "headers"

  interface WebSocketOptions {
    url: string;
    protocol?: string;
    keepalive?: number;
    headers?: Headers | Record<string, string>;
    attach?: any;
    ws?: any;
    wss?: any;
  }

  interface WebSocketCloseEvent {
    code: number;
    reason: string;
    wasClean: boolean;
  }

  interface WebSocketMessageEvent {
    data: string | ArrayBuffer;
  }

  interface WebSocketEvent {
    // Generic event object
  }

  interface WebSocketErrorEvent {
    message?: string;
  }

  type WebSocketEventListener = (event: any) => void;

  class WebSocket {
    constructor(url: string, protocol?: string);
    constructor(options: WebSocketOptions);

    binaryType: "arraybuffer";
    readonly bufferedAmount: number;
    readonly extensions: string;
    readonly protocol: string;
    readonly readyState: number;
    readonly url: string;

    onclose?(event: WebSocketCloseEvent): void;
    onerror?(event: WebSocketErrorEvent): void;
    onmessage?(event: WebSocketMessageEvent): void;
    onopen?(event: WebSocketEvent): void;

    addEventListener(event: "close", listener: (event: WebSocketCloseEvent) => void): void;
    addEventListener(event: "error", listener: (event: WebSocketErrorEvent) => void): void;
    addEventListener(event: "message", listener: (event: WebSocketMessageEvent) => void): void;
    addEventListener(event: "open", listener: (event: WebSocketEvent) => void): void;

    removeEventListener(event: "close" | "error" | "message" | "open", listener: WebSocketEventListener): void;

    close(code?: number, reason?: string): void;
    send(data: string | ArrayBuffer | ArrayBufferView): void;
  }

  export default WebSocket;
}