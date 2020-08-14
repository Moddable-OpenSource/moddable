declare module "pins/smbus" {
  type TypedArray =
    | Int8Array
    | Uint8Array
    | Int16Array
    | Uint16Array
    | Int32Array
    | Uint32Array
    | Uint8ClampedArray
    | Float32Array
    | Float64Array;
  class SMBus {
    public constructor(dictionary: {
      address: number;
      scl?: number;
      sda?: number;
      hz?: number;
      timeout?: number;
    });
    public constructor(port: number[], pin: number, mode: number);
    public close(): void;
    public read(count: number, buffer?: ArrayBuffer): void;
    public write(
      first: any,
      ...valuesOrStop: (
        | number
        | string
        | (number | string)[]
        | TypedArray
        | boolean
      )[]
    ): void;
    public readByte(register: number): number;
    public readWord(register: number): number;
    public readBlock(
      register: number,
      count: number,
      buffer?: ArrayBuffer
    ): Uint8Array;
    public writeByte(register: number, value: number): void;
    public writeWord(register: number, value: number): void;
    public writeBlock(
      register: number,
      first: any,
      ...valuesOrStop: (any | boolean)[]
    ): void;
  }
  export { SMBus as default };
}
