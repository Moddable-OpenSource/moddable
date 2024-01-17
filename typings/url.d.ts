/*
* Copyright (c) 2024 Satoshi Tanaka
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

declare module "url" {

    class URL {
        constructor(url: string, base?: string)
        hash: string;
        host: string;
        hostname: string;
        href: string;
        readonly origin: string;
        password: string;
        pathname: string;
        port: string;
        protocol: string;
        search: string;
        readonly searchParams: URLSearchParams;
        username: string;
        toJSON(): string;
        toString(): string;
    }

    class URLSearchParams implements Iterable<[string, string]>{
        constructor(it?: string[][] | Record<string, string> | string)
        readonly length: number;
        append(name: string, value: string): void;
        delete(name: string, value?: string): void;
        get(name: string): string | null;
        getAll(name: string): string[];
        has(name: string, value?: string): boolean;
        set(name: string, value: string): void;
        toString(): string;
        updatePairs(): void;
        updateParts(): void;
        [Symbol.iterator](): IterableIterator<[string, string]>;
    }
    export { URL, URLSearchParams }
}
