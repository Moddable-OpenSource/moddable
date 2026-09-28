declare module "embedded:io/socket/tcp/tls" {
  import TCP, { Options as TCPOptions } from "embedded:io/socket/tcp"

  export type TLSVersion = "TLSv1.1" | "TLSv1.2" | "TLSv1.3"

  export type TLSOptions = {
    /** Server Name Indication (SNI) and the name certificates are validated against. */
    host: string
    minimumVersion?: TLSVersion
    maximumVersion?: TLSVersion
    /** Application-Layer Protocol Negotiation (ALPN). */
    applicationLayerProtocol?: string | ByteBuffer | Array<string | ByteBuffer>
    maximumFragmentLength?: number
    ca?: ByteBuffer | ByteBuffer[]
    clientKey?: ByteBuffer | ByteBuffer[]
    clientCertificate?: ByteBuffer | ByteBuffer[]

    // Moddable SDK extensions
    /** Trace TLS protocol activity. Defaults to `false`. */
    trace?: boolean
    /** Verify the server's certificate chain. Defaults to `true`. Never disable in production. */
    verify?: boolean
  }

  /** Extends the TCP socket's options object. Listed explicitly so the callbacks
      can be typed with `this` as the TLS socket rather than the TCP socket. */
  export type Options = {
    address: string
    port: number
    noDelay?: boolean
    format?: "number" | "buffer"
    target?: any
    onReadable?: (this: TLS, bytes: number) => void
    onWritable?: (this: TLS, bytes: number) => void
    onError?: (this: TLS) => void

    tls: TLSOptions
    /** The TCP socket this TLS socket runs over. A Moddable SDK extension. */
    TCP: { io: new (options: TCPOptions) => TCP } & Partial<TCPOptions>
  }

  class TLS implements Disposable {
    target?: any
    constructor(options: Options)
    close(): void
    read(): number | ArrayBuffer
    read(byteLength: number): ArrayBuffer
    read(buffer: ByteBuffer): number
    write(buffer: ByteBuffer, options?: { more?: boolean, byteLength?: number }): number
    get format(): "number" | "buffer"
    set format(value: "number" | "buffer")
    [Symbol.dispose](): void
  }
  export default TLS;
}
