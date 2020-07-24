declare module "dns/server" {
  class Server {
    constructor(callback: (message: number, value?: string) => string);
    close(): void;
  }
  export {Server as default};
}
