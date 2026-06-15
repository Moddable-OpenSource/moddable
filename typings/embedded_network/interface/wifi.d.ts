/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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
	import type {PortSpecifier} from "embedded:io/_common";

	export type WiFiChangedProperty = "connection" | "address";

	export interface WiFiConstructorOptions {
		port?: PortSpecifier;
		onChanged?: (property: WiFiChangedProperty) => void;
	}

	export interface WiFiScanResult {
		SSID: string;
		channel: number;
		RSSI: number;
		BSSID?: string;
	}

	export interface WiFiScanOptions {
		onFound?: (result: WiFiScanResult) => void;
		onComplete?: () => void;
	}

	export interface WiFiConnectOptions {
		SSID: string;
		password?: string;
	}

	export interface WiFiStaticAddress {
		address: string;
		mask: string;
		gateway: string;
	}

	export interface WiFiConfigureOptions {
		hostname?: string;
		static?: WiFiStaticAddress | false;
	}

	class WiFi {
		constructor(options: WiFiConstructorOptions);

		close(): void;
		scan(options: WiFiScanOptions): void;
		connect(options: WiFiConnectOptions): void;
		disconnect(): void;
		configure(options: WiFiConfigureOptions): void;

		readonly connection: number;
		readonly address: string | undefined;
		readonly MAC: string | undefined;
		readonly SSID: string | undefined;
		readonly RSSI: number | undefined;
	}

	export default WiFi;
}
