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

// handlers that complete a WebSocket handshake. Spread them into a route; in its onDone, detach the socket and
// attach a WebSocket to it. To choose a subprotocol, set the connection's protocol before the response is sent
declare module "embedded:network/http/server/route/ws/handshake" {
    import type { HTTPConnectionHandlers } from "embedded:network/http/server";

    const route: HTTPConnectionHandlers;
    export default route;
}
