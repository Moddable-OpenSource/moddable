declare module "telnet" {
  class Telnet {
    constructor({port: number});
    close(): void;
  }
  export {Telnet as default};
}
