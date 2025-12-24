declare module "embedded:provider/builtin" {

  const device: {
    readonly network: {
      http: {
        io: typeof import("embedded:network/http/client").default;
        protocol: "http";
      };
      https: {
        io: typeof import("embedded:network/http/client").default;
        protocol: "https";
      };
      ws: {
        io: typeof import("embedded:network/websocket/client").default;
        secure: false;
      };
      wss: {
        io: typeof import("embedded:network/websocket/client").default;
        secure: true;
      };
    };
  }

  export default device;
}
