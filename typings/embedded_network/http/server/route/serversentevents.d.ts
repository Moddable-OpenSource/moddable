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

// handlers that accept a Server-Sent Events request. Spread them into a route and add onConnection, which
// is called once the event stream is open, with a connection whose write sends an event
declare module "embedded:network/http/server/route/serversentevents" {
    import type { HTTPConnection, HTTPConnectionHandlers } from "embedded:network/http/server";

    export interface ServerSentEvent {
        data?: string;
        event?: string;
        id?: string;
    }

    export interface ServerSentEventsConnection extends Omit<HTTPConnection, "write"> {
        write(event?: ServerSentEvent): void;			// no event sends a keep-alive comment
    }

    export interface ServerSentEventsRoute extends HTTPConnectionHandlers {
        onConnection?(this: ServerSentEventsConnection): void;
    }

    const route: HTTPConnectionHandlers;
    export default route;
}

declare module "embedded:network/http/server" {
    interface HTTPRouteTypes {
        serversentevents: import("embedded:network/http/server/route/serversentevents").ServerSentEventsRoute;
    }
}
