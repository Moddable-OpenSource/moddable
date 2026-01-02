declare module "embedded:network/http/server/options/websocket" {
    import type { HTTPServerRoute } from "embedded:network/http/server";

    export interface HTTPServerRouteWebSocket extends HTTPServerRoute {}

    const route: HTTPServerRouteWebSocket;
    export default route;
}
