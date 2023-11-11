declare module "embedded:network/mqtt/client" {
  import type { Buffer } from "embedded:io/_common"

  interface onReadableOptions {
    more: boolean,
    topic?: string,
    QoS?: number,
    byteLength?: number,
  }

  interface onControlMessage {
    operation: number, // MQTTClient.CONNACK, etc...
    id: number, 
    payload?: Buffer, // for SUBACK
  }

  interface Will {
    topic?: string,
    message?: string|Buffer,
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
    will?: any,
    keepAlive?: number, // milliseconds
    onReadable?: (count: number, options: onReadableOptions) => void,
    onWritable?: (count: number) => void,
    onControl?: (opcode: number, message: onControlMessage) => void,
    onClose?: () => void,
    onError?: (error: any) => void,
    dns: Record<string, any>,
    socket: Record<string, any>,
  }

  interface WriteOptions {
    operation?: number,
    id?: number,
    topic: string,
    QoS?: number,
    retain?: boolean,
    duplicate?: boolean,
    byteLength?: number,
    items?: any[], // being lazy here
  }

  export default class MQTTClient {
    constructor(options: Options)
    read(count: number): ArrayBuffer
    write(data: Buffer, options: WriteOptions): number
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
  }
}
