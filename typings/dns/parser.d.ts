declare module "dns/parser" {
  class Parser {
    constructor(buffer: ArrayBuffer);
    get id(): number;
    get flags(): number;
    get questions(): number;
    get answers(): number;
    get authorities(): number;
    get additionals(): number;
    // TODO: not use unknown
    question(index: number): unknown;
    answer(index: number): unknown;
    authority(index: number): unknown;
    additional(index: number): unknown;
  }
  export {Parser as default};
}
