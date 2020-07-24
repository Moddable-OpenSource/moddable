declare module "ping" {
  class Ping {
    constructor(
      options: {
        host: string,
        id: number,
        interval?: number
      },
      callback: (
        message: number,
        value?: number,
        etc?: any
      ) => void
    );
    close(): void;
  }
  export {Ping as default};
}
