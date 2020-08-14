import type I2C from "pins/i2c";
declare module "pins/smbus" {
  class SMBus extends I2C {
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
