declare module "base64" {
  var Base64: {
    decode: (str: string) => ArrayBuffer;
    encode: (source: ArrayBuffer | string) => string;
  }
  export {Base64 as default};
}
