declare module "hex" {
  var Hex: {
    toBuffer: (str: string, separator?: string) => ArrayBuffer;
    toString: (buffer: ArrayBuffer, separator?: string) => string;
  }
  export {Hex as default};
}
