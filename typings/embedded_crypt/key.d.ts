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

declare module "embedded:crypt/key" {
  export type KeyFormat = "pkcs1" | "spki" | "pkcs8" | "jwk" | "sec1" | "raw";
  export type KeyType = "public" | "private";
  export type KeyAlgorithm = "RSA" | "EC";
  export type NamedCurve = "P-256" | "P-384" | "X25519";
  interface RSAJsonWebKey {
    kty: "RSA";
    n: string;
    e: string;
    d?: string;
    p?: string;
    q?: string;
    dp?: string;
    dq?: string;
    qi?: string;
  }
  interface ECJsonWebKey {
    kty: "EC";
    crv: "P-256" | "P-384";
    x: string;
    y: string;
    d?: string;
  }
  interface OKPJsonWebKey {
    kty: "OKP";
    crv: "X25519";
    x: string;
    d?: string;
  }
  export type JsonWebKey = RSAJsonWebKey | ECJsonWebKey | OKPJsonWebKey;
  interface RSAKeyOptions {
    algorithm: "RSA";
    format: "pkcs1" | "spki" | "pkcs8" | "jwk";
    data: ByteBuffer | RSAJsonWebKey;
    extractable?: boolean;
  }
  interface ECKeyOptions {
    algorithm: "EC";
    format: "spki" | "pkcs8" | "jwk" | "sec1" | "raw";
    data: ByteBuffer | ECJsonWebKey | OKPJsonWebKey;
    namedCurve?: NamedCurve;
    extractable?: boolean;
  }
  interface ECGenerateOptions {
    algorithm: "EC";
    namedCurve: NamedCurve;
    extractable?: boolean;
  }
  export type KeyOptions = RSAKeyOptions | ECKeyOptions | ECGenerateOptions;
  class Key implements Disposable {
    constructor(options: KeyOptions);
    static supports(options: Partial<KeyOptions>): boolean;
    extract(format: "jwk"): JsonWebKey;
    extract(format: "pkcs1" | "spki" | "pkcs8" | "sec1" | "raw"): ArrayBuffer;
    close(): void;
    readonly algorithm: KeyAlgorithm;
    readonly type: KeyType;
    readonly keySize: number;
    readonly namedCurve: NamedCurve | undefined;
    readonly extractable: boolean;
    [Symbol.dispose](): void;
  }
  export default Key;
}
