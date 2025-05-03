declare module "embedded:network/http/server/options/webpage" {
    import type { HTTPServerRoute } from "embedded:network/http/server";
    import type { Buffer } from "embedded:io/_common";

    export interface HTTPServerRouteWebPage extends HTTPServerRoute {
        data: Buffer | string;
        contentType?: string;
        headers?: Map<string, string | string[]>;
        status?: number;
    }
    const route: HTTPServerRouteWebPage;
    export default route;
}