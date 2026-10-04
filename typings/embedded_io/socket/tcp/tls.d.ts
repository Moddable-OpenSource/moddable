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

  export type Format = "number" | "buffer"

  /*
    The socket property of a network client provider, and of the options of a network client.
    The client creates its socket with io, passing the other properties as options.
    For a secure connection io is TLS, tls has the TLS options (the client supplies host),
    and TCP is the socket TLS runs over.
  */
  export type ClientSocket = {
    io: typeof TCP | typeof TLS
    tls?: Partial<TLSOptions>
    TCP?: { io: typeof TCP, [option: string]: unknown }
    [option: string]: unknown
  }

  /** Extends the TCP socket's options object. Listed explicitly so the callbacks
      can be typed with `this` as the TLS socket rather than the TCP socket. */
  export type Options<F extends Format = Format> = {
    address: string
    port: number
    noDelay?: boolean
    format?: F
    target?: any
    onReadable?: (this: TLS<F>, bytes: number) => void
    onWritable?: (this: TLS<F>, bytes: number) => void
    onError?: (this: TLS<F>) => void

    tls: TLSOptions
    /** The TCP socket this TLS socket runs over. A Moddable SDK extension. */
    TCP: { io: new (options: TCPOptions) => TCP } & Partial<TCPOptions>
  }

  /*
    F is the format named in the options when the socket is created. Naming it there
    (format: "buffer") fixes the type read() with no argument returns:
    a byte for "number", an ArrayBuffer for "buffer".
    When the options name no format, read() is typed as returning either, because the
    format can be changed later through the format property.
  */
  interface TLS<F extends Format = Format> extends Disposable {
    target?: any
    close(): void
    readonly remoteAddress: string | undefined
    readonly remotePort: number | undefined
    read(): F extends "number" ? number : ArrayBuffer
    read(byteLength: number): ArrayBuffer
    read(buffer: ByteBuffer): number
    write(buffer: ByteBuffer, options?: { more?: boolean, byteLength?: number }): number
    get format(): F
    set format(value: Format)
    [Symbol.dispose](): void
  }
  interface TLSConstructor {
    new <F extends Format = Format>(options: Options<F>): TLS<F>
    readonly prototype: TLS
  }
  const TLS: TLSConstructor
  export default TLS;
}
