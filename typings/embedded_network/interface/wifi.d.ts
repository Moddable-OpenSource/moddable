/*
 * Copyright (c) 2025  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

declare module "embedded:network/interface/wifi" {
	export interface WiFiConstructorOptions {
		onChanged?: () => void;
		port?: string;
	}

	export interface WiFiConnectOptions {
		SSID?: string;
		password?: string;
		BSSID?: string;
	}

	export interface WiFiAccessPoint {
		SSID: string;
		RSSI: number;
		channel: number;
		security: string;
		BSSID?: string;
	}

	export interface WiFiScanOptions {
		onFound: (accessPoint: WiFiAccessPoint) => void;
		onDone: () => void;
	}

	export default class WiFi {
		constructor(options?: WiFiConstructorOptions);
		close(): void;
		scan(options: WiFiScanOptions): void;
		connect(options?: WiFiConnectOptions): void;
		disconnect(): void;
		get connection(): number;
		get address(): string;
		get SSID(): string;
		get BSSID(): string;
		get RSSI(): number;
		get channel(): number;
	}
}
