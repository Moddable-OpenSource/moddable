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

declare module "web/eventsource" {
  import Headers from "headers"

  interface EventSourceInit {
    method?: string;
    headers?: Headers | Record<string, string>;
    body?: string | ArrayBuffer;
  }

  interface EventSourceEvent {
    type: string;
  }

  interface EventSourceMessageEvent extends EventSourceEvent {
    data: string;
    origin: string;
    lastEventID: string;
  }

  interface EventSourceErrorEvent extends EventSourceEvent {
    status?: number;
    statusText?: string;
  }

  type EventSourceEventListener = (event: any) => void;

  class EventSource {
    constructor(url: string, options?: EventSourceInit);

    static readonly CONNECTING: 0;
    static readonly OPEN: 1;
    static readonly CLOSED: 2;

    readonly CONNECTING: 0;
    readonly OPEN: 1;
    readonly CLOSED: 2;

    readonly readyState: number;
    readonly url: string;

    onerror?(event: EventSourceErrorEvent): void;
    onopen?(event: EventSourceEvent): void;
    onmessage?(event: EventSourceMessageEvent): void;

    addEventListener(type: "open", listener: (event: EventSourceEvent) => void): void;
    addEventListener(type: "error", listener: (event: EventSourceErrorEvent) => void): void;
    addEventListener(type: "message", listener: (event: EventSourceMessageEvent) => void): void;
    addEventListener(type: string, listener: (event: EventSourceMessageEvent) => void): void;

    removeEventListener(type: string, listener: EventSourceEventListener): void;

    close(): void;
  }

  export default EventSource;
}
