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

declare module "embedded:network/dnssd" {
    import type UDP from "embedded:io/socket/udp";

    export interface DNSSDSocketOptions {
        io: typeof UDP;
        address?: string;
        multicast?: string;
        timeToLive?: number;
    }

    export interface DNSSDOptions {
        socket: DNSSDSocketOptions;
        target?: any;
    }

    export interface ClaimOptions {
        host: string;
        onReady?(this: ClaimHandle): void;
        onError?(this: ClaimHandle): void;
    }

    export interface ClaimHandle extends Disposable {
        close(): void;
    }

    export interface ServiceObject {
        name: string;
        host: string;
        address: string;
        port: number;
        txt?: Map<string, string | undefined>;
    }

    export interface DiscoverOptions {
        serviceType: string;
        onFound?(this: DiscoverHandle, service: ServiceObject): void;
        onUpdate?(this: DiscoverHandle, service: ServiceObject): void;
        onLost?(this: DiscoverHandle, service: ServiceObject): void;
    }

    export interface DiscoverHandle extends Disposable {
        close(): void;
    }

    export interface AdvertiseOptions {
        serviceType: string;
        host: string;
        name?: string;
        port: number;
        txt?: Map<string, string | ByteBuffer>;
    }

    export interface AdvertiseHandle extends Disposable {
        close(): void;
        updateTXT(txt: Map<string, string | ByteBuffer>): void;
    }

    class DNSSD implements Disposable {
        target?: any;
        constructor(options: DNSSDOptions);
        close(): void;
        claim(options: ClaimOptions): ClaimHandle;
        discover(options: DiscoverOptions): DiscoverHandle;
        advertise(options: AdvertiseOptions): AdvertiseHandle;
        [Symbol.dispose](): void;
    }
    export default DNSSD;
}
