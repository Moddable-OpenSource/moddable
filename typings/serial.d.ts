declare module "serial" {
  class Serial {
    public constructor()
    public setTimeout(ms: number): void
    public setBaudrate(baud: number): void

    public available(): boolean
    public flush(): void

    public readBytes(bytes: number): string
    public readBytes(buffer: ArrayBuffer, bytes?: number): void
    public readLine(): string
    public readLineUntil(terminator: string): string
    public readBytesUntil(character: string, bytes: number): string
    public readBytesUntil(buffer: ArrayBuffer, character: string, bytes: number): void

    public write(msg: ArrayBufferLike): void
    public write(msg: ArrayBufferLike, from: number): void
    public write(msg: ArrayBufferLike, from: number, to: number): void

    public write(): void
    public writeLine(line: string): void
    public onDataReceived: (str: string, len: number) => void
    public poll(dictionary: { terminators: string | string[]; trim: number; chunkSize: number }): void
    public poll(): void
  }
  export { Serial as default }
}
