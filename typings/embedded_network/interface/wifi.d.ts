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

	export type WiFiSecurity = "none" | "wep" | "wpa_psk" | "wpa2_psk" | "wpa_wpa2_psk" | "wpa3_psk" | "wpa2_wpa3_psk" | "wpa2_enterprise" | "unknown" | (string & {});

	export interface WiFiConstructorOptions {
		port?: PortSpecifier;
		onChanged?: (this: WiFi, property: WiFiChangedProperty) => void;
	}

	export interface WiFiScanResult {
		SSID: string;
		BSSID: string;
		RSSI: number;
		channel: number;
		security: WiFiSecurity;
	}

	export interface WiFiScanOptions {
		onFound: (this: WiFi, result: WiFiScanResult) => void;
		onComplete?: (this: WiFi) => void;
		channel?: number;
		frequency?: 2.4 | 5;
		secure?: boolean;
	}

	export type WiFiConnectOptions = ({ SSID: string; BSSID?: string } | { SSID?: string; BSSID: string }) & {
		password?: string;
		secure?: boolean;			// true to disallow connecting to an open access point
		channel?: number;
	};

	export interface WiFiStaticAddress {
		address: string;
		mask: string;
		gateway: string;
	}

	// Moddable SDK extension
	export interface WiFiConfigureOptions {
		hostname?: string;
		static?: WiFiStaticAddress | false;
	}

	class WiFi implements Disposable {
		constructor(options: WiFiConstructorOptions);

		close(): void;
		scan(options: WiFiScanOptions): void;
		connect(options: WiFiConnectOptions): void;
		disconnect(): void;
		configure(options: WiFiConfigureOptions): void;		// Moddable SDK extension

		readonly connection: number;
		readonly address: string | undefined;
		readonly MAC: string | undefined;
		readonly SSID: string | undefined;
		readonly BSSID: string | undefined;
		readonly RSSI: number | undefined;
		readonly channel: number | undefined;
		[Symbol.dispose](): void;
	}

	export default WiFi;
}
