// the manifest that supplies this module includes the manifests of these configurations
/// <reference path="../../config/mqttclient.d.ts" />

declare module "embedded:network/mqtt/client" {
  import type TCP from "embedded:io/socket/tcp";
  import type TLS from "embedded:io/socket/tcp/tls";
  import type { ClientSocket } from "embedded:io/socket/tcp/tls";
  import type { DNSResolverProvider } from "embedded:network/dns/resolver/udp";

  export interface MQTTClientProvider {
    io: typeof MQTTClient,
    dns: DNSResolverProvider,
    socket: ClientSocket,
  }

  interface onReadableOptions {
    more: boolean,
    topic?: string,
    QoS?: number,
    byteLength?: number,
  }

  interface onControlMessage {
    operation: number, // MQTTClient.CONNACK, etc...
    id: number, 
    payload?: ByteBuffer, // for SUBACK
  }

  interface Will {
    topic?: string,
    message?: string | ByteBuffer,
    QoS?: number,
    retain?: boolean,
  }

  interface Options {
    host: string,
    port?: number,
    user?: string,
    password?: string,
    id?: string,
    clean?: boolean,
    will?: Will,
    keepAlive?: number, // milliseconds
    onReadable?: (this: MQTTClient, count: number, options: onReadableOptions) => void,
    onWritable?: (this: MQTTClient, count: number) => void,
    onControl?: (this: MQTTClient, opcode: number, message: onControlMessage) => void,
    onClose?: (this: MQTTClient) => void,
    onError?: (this: MQTTClient, error: any) => void,
    target?: any,
    dns: DNSResolverProvider,
    socket: ClientSocket,
  }

  interface PublishOptions {
    operation?: typeof MQTTClient.PUBLISH,
    id?: number,
    topic: string,
    QoS?: number,
    retain?: boolean,
    duplicate?: boolean,
    byteLength?: number, // total payload, when larger than this first fragment
  }

  interface SubscribeOptions {
    operation: typeof MQTTClient.SUBSCRIBE,
    id?: number,
    items: { topic: string, QoS?: number }[],
  }

  interface UnsubscribeOptions {
    operation: typeof MQTTClient.UNSUBSCRIBE,
    id?: number,
    items: { topic: string }[],
  }

  interface AcknowledgeOptions {
    operation: typeof MQTTClient.PUBACK | typeof MQTTClient.PUBREC | typeof MQTTClient.PUBREL | typeof MQTTClient.PUBCOMP,
    id: number,
  }

  interface SessionOptions {
    operation: typeof MQTTClient.PINGREQ | typeof MQTTClient.DISCONNECT,
  }

  type WriteOptions = PublishOptions | SubscribeOptions | UnsubscribeOptions | AcknowledgeOptions | SessionOptions;

  class MQTTClient implements Disposable {
    target?: any
    constructor(options: Options)
    read(count?: number): ArrayBuffer
    read(buffer: ByteBuffer): number
    write(data: ByteBuffer): number // continues the publish in progress
    write(data: ByteBuffer | null, options: WriteOptions): number
    close(): void

    static CONNECT: 1
    static CONNACK: 2
    static PUBLISH: 3
    static PUBACK: 4
    static PUBREC: 5
    static PUBREL: 6
    static PUBCOMP: 7
    static SUBSCRIBE: 8
    static SUBACK: 9
    static UNSUBSCRIBE: 10
    static UNSUBACK: 11
    static PINGREQ: 12
    static PINGRESP: 13
    static DISCONNECT: 14
    [Symbol.dispose](): void
  }
  export default MQTTClient;
}
