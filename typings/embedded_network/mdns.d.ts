/*
* Copyright (c) 2025 Moddable Tech, Inc.
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

declare module "embedded:network/mdns" {
	export interface DNSSDOptions {
		socket: {
			io: any;
			[key: string]: any;
		};
	}

	export interface ClaimOptions {
		host: string;	// without .local suffix
		onReady?: (this: ClaimInstance) => void;
		onError?: (this: ClaimInstance, error?: any) => void;
	}

	export interface AdvertiseOptions {
		serviceType: string;	// e.g., "_http._tcp"
		host: string;
		port: number;
		txt?: Map<string, string | undefined>;
		instanceName?: string;	// defaults to host if not provided
		onError?: (this: AdvertiseInstance, error?: any) => void;
	}

	export interface DiscoverOptions {
		serviceType: string;	// e.g., "_http._tcp"
		onFound?: (this: DiscoverInstance, service: DiscoveredService) => void;
		onUpdate?: (this: DiscoverInstance, service: DiscoveredService) => void;
		onLost?: (this: DiscoverInstance, service: DiscoveredService) => void;
		onError?: (this: DiscoverInstance, error?: any) => void;
	}

	export interface DiscoveredService {
		name: string;
		host: string;
		address: string;
		port: number;
		txt?: Map<string, string | undefined>;
	}

	export interface ClaimInstance extends Disposable {
		close(): void;
	}

	export interface AdvertiseInstance extends Disposable {
		close(): void;
		updateTXT(txt?: Map<string, string | undefined>): void;
	}

	export interface DiscoverInstance extends Disposable {
		close(): void;
	}

	class DNSSD implements Disposable {
		constructor(options: DNSSDOptions);
		close(): void;
		claim(options: ClaimOptions): ClaimInstance;
		advertise(options: AdvertiseOptions): AdvertiseInstance;
		discover(options: DiscoverOptions): DiscoverInstance;
		[Symbol.dispose](): void;
	}
	export default DNSSD;
}