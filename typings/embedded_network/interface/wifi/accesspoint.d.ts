/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

declare module "embedded:network/interface/wifi/accesspoint" {
	export type WiFiAccessPointChangedProperty = "connection";

	export type WiFiAccessPointAuthentication = "none" | "wpa_psk" | "wpa2_psk" | "wpa_wpa2_psk" | "wpa3_psk" | "wpa2_wpa3_psk";

	export interface WiFiAccessPointStation extends Disposable {
		close(): void;

		readonly MAC: string;
		readonly address: string | undefined;
	}

	export interface WiFiAccessPointConstructorOptions {
		SSID: string;
		password?: string;
		authentication?: WiFiAccessPointAuthentication;
		channel?: number;
		hidden?: boolean;
		max?: number;
		interval?: number;
		onChanged?(this: WiFiAccessPoint, property: WiFiAccessPointChangedProperty): void;
		onConnect?(this: WiFiAccessPoint, station: WiFiAccessPointStation): void;
		onDisconnect?(this: WiFiAccessPoint, station: WiFiAccessPointStation): void;
	}

	export interface WiFiAccessPointConfigureOptions {
		portal?: string;
	}

	class WiFiAccessPoint implements Disposable {
		constructor(options: WiFiAccessPointConstructorOptions);

		close(): void;
		configure(options: WiFiAccessPointConfigureOptions): void;

		readonly connection: number;
		readonly address: string | undefined;
		readonly MAC: string | undefined;
		readonly SSID: string | undefined;
		readonly channel: number | undefined;
		[Symbol.dispose](): void;
	}

	export default WiFiAccessPoint;
}
