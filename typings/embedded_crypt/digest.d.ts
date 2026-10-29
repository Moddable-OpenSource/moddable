/*
* Copyright (c) 2026 Moddable Tech, Inc.
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

declare module "embedded:crypt/digest" {
  export type DigestAlgorithm = "MD5" | "SHA-1" | "SHA-224" | "SHA-256" | "SHA-384" | "SHA-512";

  interface DigestOptions {
    algorithm: DigestAlgorithm;
  }

  class Digest implements Disposable {
    constructor(options: DigestOptions);
    static supports(options: Partial<DigestOptions>): boolean;
    write(data: ByteBuffer): void;
    read(): ArrayBuffer;
    read(buffer: ByteBuffer): number;
    reset(): void;
    close(): void;
    readonly blockSize: number;
    readonly outputSize: number;
    [Symbol.dispose](): void;
  }

  export default Digest;
}
