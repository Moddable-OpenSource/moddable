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

declare module "embedded:network/ntp/client" {
	import type UDP from "embedded:io/socket/udp";
	import type Resolver from "embedded:network/dns/resolver/udp";
	import type { DNSResolverProvider } from "embedded:network/dns/resolver/udp";

	export interface NTPClientProvider {
		io: typeof NTP;
		dns: DNSResolverProvider;
		socket: { io: typeof UDP; [option: string]: unknown };
		servers: string[];
	}

	export interface NTPOptions<T = any> {
		servers: string[];
		socket: { io: typeof UDP };
		dns: { io: typeof Resolver };
		target?: T;
	}
	
	export type NTPCallback<T = any> = (this: NTP<T>, error: Error | null, time: number) => void;
	
	class NTP<T = any> {
		target?: T;
		constructor(options: NTPOptions<T>);
		close(): void;
		getTime(callback: NTPCallback<T>): void;
	}
	interface NTP<T = any> extends Disposable {}
	export default NTP;
}
