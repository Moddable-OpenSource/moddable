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

declare module "embedded:crypt/signature" {
  import Key from "embedded:crypt/key";
  export type SignatureAlgorithm = "RSASSA-PKCS1-v1_5" | "RSA-PSS" | "ECDSA";
  export type SignatureHash = "MD5" | "SHA-1" | "SHA-224" | "SHA-256" | "SHA-384" | "SHA-512";
  interface RSASignatureOptions {
    algorithm: "RSASSA-PKCS1-v1_5" | "RSA-PSS";
    key: Key;
    hash: SignatureHash;
    saltLength?: number;
  }
  interface ECDSASignatureOptions {
    algorithm: "ECDSA";
    key: Key;
  }
  export type SignatureOptions = RSASignatureOptions | ECDSASignatureOptions;
  class Signature implements Disposable {
    constructor(options: SignatureOptions);
    static supports(options: Partial<SignatureOptions>): boolean;
    sign(digest: ByteBuffer): ArrayBuffer;
    verify(digest: ByteBuffer, signature: ByteBuffer): boolean;
    close(): void;
    readonly algorithm: SignatureAlgorithm;
    readonly hash: SignatureHash | undefined;
    readonly outputSize: number;
    [Symbol.dispose](): void;
  }
  export default Signature;
}
