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
    }

    export interface ClaimOptions {
        host: string;
        onReady?(): void;
        onError?(): void;
    }

    export interface ClaimHandle {
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
        onFound?(service: ServiceObject): void;
        onUpdate?(service: ServiceObject): void;
        onLost?(service: ServiceObject): void;
    }

    export interface DiscoverHandle {
        close(): void;
    }

    export interface AdvertiseOptions {
        serviceType: string;
        host: string;
        name?: string;
        port: number;
        txt?: Map<string, string | ByteBuffer>;
    }

    export interface AdvertiseHandle {
        close(): void;
        updateTXT(txt: Map<string, string | ByteBuffer>): void;
    }

    export default class DNSSD {
        constructor(options: DNSSDOptions);
        close(): void;
        claim(options: ClaimOptions): ClaimHandle;
        discover(options: DiscoverOptions): DiscoverHandle;
        advertise(options: AdvertiseOptions): AdvertiseHandle;
    }
}
