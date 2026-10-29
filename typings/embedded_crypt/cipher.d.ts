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

declare module "embedded:crypt/cipher" {
  import Key from "embedded:crypt/key";

  export type CipherAlgorithm = "AES-ECB" | "AES-CBC" | "AES-CTR" | "AES-GCM" | "ChaCha20" | "ChaCha20-Poly1305";
  export type RSACipherAlgorithm = "RSA-OAEP" | "RSAES-PKCS1-v1_5";
  export type CipherDirection = "encrypt" | "decrypt";
  export type CipherPadding = "none" | "pkcs7";

  interface CipherOptions {
    algorithm: CipherAlgorithm;
    key: ByteBuffer;
    direction: CipherDirection;
    iv?: ByteBuffer;
    additionalData?: ByteBuffer;
    tagLength?: number;
    padding?: CipherPadding;
  }
  interface RSACipherOptions {
    algorithm: RSACipherAlgorithm;
    key: Key;
    direction: CipherDirection;
    hash?: "MD5" | "SHA-1" | "SHA-224" | "SHA-256" | "SHA-384" | "SHA-512";
    label?: ByteBuffer;
  }
  interface CipherResetOptions {
    iv?: ByteBuffer;
    additionalData?: ByteBuffer;
    tagLength?: number;
  }
  interface CipherProcessOptions {
    more?: boolean;
    output?: ByteBuffer;
  }
  interface CipherProcessOptionsWithOutput extends CipherProcessOptions {
    output: ByteBuffer;
  }

  class Cipher implements Disposable {
    constructor(options: CipherOptions | RSACipherOptions);
    static supports(options: Partial<CipherOptions | RSACipherOptions> & {more?: boolean}): boolean;
    encrypt(data: ByteBuffer | undefined, options: CipherProcessOptionsWithOutput): number;
    encrypt(data?: ByteBuffer, options?: CipherProcessOptions): ArrayBuffer;
    decrypt(data: ByteBuffer | undefined, options: CipherProcessOptionsWithOutput): number;
    decrypt(data?: ByteBuffer, options?: CipherProcessOptions): ArrayBuffer;
    reset(options?: CipherResetOptions): void;
    close(): void;
    readonly blockSize: number;
    readonly keySize: number;
    readonly tagLength: number;
    [Symbol.dispose](): void;
  }

  export default Cipher;
}
