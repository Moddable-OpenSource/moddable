declare module "embedded:io/socket/tlssocket" {
  import { Options as TCPOptions } from "embedded:io/socket/tcp"
  export type Options = TCPOptions & {
    host: string
    secure: Record<string, any> // should be called "tls" according to std?
  }
  export default class TLSSocket {
    constructor(options: Options) 
    close(): undefined
    read(count: number|ArrayBufferLike) : undefined|ArrayBufferLike
    write(buffer: ArrayBufferLike) : number
    set format(format: string) 
    get format() : string
  }
}

