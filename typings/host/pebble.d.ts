/*
* Copyright (c) 2025-2026 Moddable Tech, Inc.
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

// the host provider of Pebble, build/devices/pebble/host/main.js. Its network clients are Pebble's
// own implementations of the ECMA-419 classes. They connect through the phone, so they take no socket
// or resolver: the HTTP client takes the protocol and the WebSocket client whether it is secure.
/// <reference path="../embedded_provider/builtin.d.ts" />

declare module "embedded:provider/builtin" {
	import type HTTPClient from "embedded:network/http/client";
	import type { ClientOptions } from "embedded:network/http/client";
	import type WebSocketClient from "embedded:network/websocket/client";
	import type { Directory } from "embedded:storage/files";
	import type keyValue from "embedded:storage/key-value";
	import type Touch from "embedded:sensor/Touch/pebble";

	interface PebbleHTTPClient {
		new (options: Omit<ClientOptions, "socket" | "dns"> & { protocol?: "http" | "https" }): HTTPClient;
		readonly prototype: HTTPClient;
	}
	interface Device {
		network: DeviceNetwork;
		info: DeviceInfo;
		files: Directory;
		keyValue: typeof keyValue;
		sensor: DeviceSensor;
	}
	interface DeviceSensor {
		Touch: typeof Touch;
	}
	interface DeviceNetwork {
		http: {
			io: PebbleHTTPClient;
			protocol: "http";
		};
		https: {
			io: PebbleHTTPClient;
			protocol: "https";
		};
		ws: {
			io: typeof WebSocketClient;
			secure: false;
		};
		wss: {
			io: typeof WebSocketClient;
			secure: true;
		};
	}
	interface DeviceInfo {
		language: string;
		serialNumber: string;
	}
}
