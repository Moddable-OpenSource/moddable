type TypedArray = Int8Array | Uint8Array | Int16Array | Uint16Array | Int32Array | Uint32Array | Uint8ClampedArray | Float32Array | Float64Array;

declare module "pins/i2c" {
  class I2C {
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
    public write(first: any, ...valuesOrStop: (number | string | (number | string)[] | TypedArray | boolean)[]): void;
  }
  export { I2C as default };
}
