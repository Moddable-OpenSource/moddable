/*
* Copyright (c) 2025 Satoshi Tanaka
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

declare module "crypt" {
    type DigestType = "MD5" | "SHA1" | "SHA224" | "SHA256" | "SHA384" | "SHA512";

    export class Digest {
        constructor(type: DigestType);
        
        write(message: string | ArrayBuffer): void;
        close(): ArrayBuffer;
        reset(): void;
        get blockSize(): number;
        get outputSize(): number;
        update(message: string | ArrayBuffer): void;
    }
}
