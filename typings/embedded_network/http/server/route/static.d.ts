/*
* Copyright (c) 2026 Moddable Tech, Inc.
*
*   This file is part of the Moddable SDK Tools.
*
*   The Moddable SDK Tools is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   The Moddable SDK Tools is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
*
*/

// handlers that respond with fixed content. Spread them into a route and add the content:
//	{ ...StaticRoute, data: "<html>…</html>", contentType: "text/html" }
declare module "embedded:network/http/server/route/static" {
    import type { HTTPConnectionHandlers } from "embedded:network/http/server";

    export interface StaticRoute extends HTTPConnectionHandlers {
        data: ByteBuffer | string;		// a string is sent with chunked transfer encoding
        contentType?: string;			// defaults to "text/html"
        headers?: Map<string, string | number>;
        status?: number;
    }

    const route: HTTPConnectionHandlers;
    export default route;
}

declare module "embedded:network/http/server" {
    interface HTTPRouteTypes {
        static: import("embedded:network/http/server/route/static").StaticRoute;
    }
}
