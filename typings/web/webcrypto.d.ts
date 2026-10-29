/*
* Copyright (c) 2025-2026 Moddable Tech, Inc.
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

declare module "webcrypto" {
  type BufferSource = ArrayBuffer | ArrayBufferView;
  type HashName = "SHA-1" | "SHA-256" | "SHA-384" | "SHA-512";
  type HashAlgorithmIdentifier = HashName | { name: HashName };
  type KeyUsage = "encrypt" | "decrypt" | "sign" | "verify" | "deriveKey" | "deriveBits" | "wrapKey" | "unwrapKey";

  interface Algorithm { name: string }
  interface AesCbcParams extends Algorithm { name: "AES-CBC"; iv: BufferSource }
  interface AesCtrParams extends Algorithm { name: "AES-CTR"; counter: BufferSource; length: number }
  interface AesGcmParams extends Algorithm { name: "AES-GCM"; iv: BufferSource; additionalData?: BufferSource; tagLength?: number }
  interface AesKeyGenParams extends Algorithm { name: "AES-CBC" | "AES-CTR" | "AES-GCM" | "AES-KW"; length: 128 | 192 | 256 }
  interface AesKeyAlgorithm extends Algorithm { length: number }
  interface HmacImportParams extends Algorithm { name: "HMAC"; hash: HashAlgorithmIdentifier; length?: number }
  interface HmacKeyGenParams extends HmacImportParams {}
  interface HmacKeyAlgorithm extends Algorithm { name: "HMAC"; hash: { name: HashName }; length: number }
  interface Pbkdf2Params extends Algorithm { name: "PBKDF2"; hash: HashAlgorithmIdentifier; salt: BufferSource; iterations: number }
  interface HkdfParams extends Algorithm { name: "HKDF"; hash: HashAlgorithmIdentifier; salt: BufferSource; info: BufferSource }
  interface RsaHashedImportParams extends Algorithm { name: "RSASSA-PKCS1-v1_5" | "RSA-PSS" | "RSA-OAEP"; hash: HashAlgorithmIdentifier }
  interface RsaHashedKeyAlgorithm extends Algorithm { modulusLength: number; publicExponent: Uint8Array; hash: { name: HashName } }
  interface RsaPssParams extends Algorithm { name: "RSA-PSS"; saltLength: number }
  interface RsaOaepParams extends Algorithm { name: "RSA-OAEP"; label?: BufferSource }
  type NamedCurve = "P-256" | "P-384";
  interface EcKeyImportParams extends Algorithm { name: "ECDSA" | "ECDH"; namedCurve: NamedCurve }
  interface EcKeyGenParams extends EcKeyImportParams {}
  interface EcKeyAlgorithm extends Algorithm { name: "ECDSA" | "ECDH"; namedCurve: NamedCurve }
  interface EcdsaParams extends Algorithm { name: "ECDSA"; hash: HashAlgorithmIdentifier }
  interface EcdhKeyDeriveParams extends Algorithm { name: "ECDH" | "X25519"; public: CryptoKey }
  interface X25519Algorithm extends Algorithm { name: "X25519" }
  type EncryptAlgorithm = AesCbcParams | AesCtrParams | AesGcmParams | RsaOaepParams;
  type ImportAlgorithm = "AES-CBC" | "AES-CTR" | "AES-GCM" | "AES-KW" | "PBKDF2" | "HKDF" | Algorithm | HmacImportParams | RsaHashedImportParams | EcKeyImportParams | X25519Algorithm | "X25519";
  type KeyFormat = "raw" | "jwk" | "spki" | "pkcs8";
  type DeriveAlgorithm = Pbkdf2Params | HkdfParams | EcdhKeyDeriveParams;

  interface CryptoKeyPair { publicKey: CryptoKey; privateKey: CryptoKey }
  interface JsonWebKey {
    kty: string;
    k?: string;
    n?: string;
    e?: string;
    d?: string;
    p?: string;
    q?: string;
    dp?: string;
    dq?: string;
    qi?: string;
    alg?: string;
    use?: string;
    key_ops?: string[];
    ext?: boolean;
  }

  class CryptoKey {
    private constructor();
    readonly type: "secret" | "public" | "private";
    readonly extractable: boolean;
    readonly algorithm: Algorithm | AesKeyAlgorithm | HmacKeyAlgorithm | RsaHashedKeyAlgorithm | EcKeyAlgorithm | X25519Algorithm;
    readonly usages: readonly KeyUsage[];
  }

  class SubtleCrypto {
    digest(algorithm: HashAlgorithmIdentifier, data: BufferSource): Promise<ArrayBuffer>;
    encrypt(algorithm: EncryptAlgorithm, key: CryptoKey, data: BufferSource): Promise<ArrayBuffer>;
    decrypt(algorithm: EncryptAlgorithm, key: CryptoKey, data: BufferSource): Promise<ArrayBuffer>;
    sign(algorithm: "HMAC" | "RSASSA-PKCS1-v1_5" | RsaPssParams | Algorithm | EcdsaParams, key: CryptoKey, data: BufferSource): Promise<ArrayBuffer>;
    verify(algorithm: "HMAC" | "RSASSA-PKCS1-v1_5" | RsaPssParams | Algorithm | EcdsaParams, key: CryptoKey, signature: BufferSource, data: BufferSource): Promise<boolean>;
    generateKey(algorithm: AesKeyGenParams | HmacKeyGenParams, extractable: boolean, usages: KeyUsage[]): Promise<CryptoKey>;
    generateKey(algorithm: EcKeyGenParams | X25519Algorithm | "X25519", extractable: boolean, usages: KeyUsage[]): Promise<CryptoKeyPair>;
    importKey(format: "jwk", keyData: JsonWebKey, algorithm: ImportAlgorithm, extractable: boolean, usages: KeyUsage[]): Promise<CryptoKey>;
    importKey(format: "raw" | "spki" | "pkcs8", keyData: BufferSource, algorithm: ImportAlgorithm, extractable: boolean, usages: KeyUsage[]): Promise<CryptoKey>;
    exportKey(format: "jwk", key: CryptoKey): Promise<JsonWebKey>;
    exportKey(format: "raw" | "spki" | "pkcs8", key: CryptoKey): Promise<ArrayBuffer>;
    deriveBits(algorithm: DeriveAlgorithm, baseKey: CryptoKey, length: number | null): Promise<ArrayBuffer>;
    deriveKey(algorithm: DeriveAlgorithm, baseKey: CryptoKey, derivedKeyType: AesKeyGenParams | HmacImportParams, extractable: boolean, usages: KeyUsage[]): Promise<CryptoKey>;
    wrapKey(format: KeyFormat, key: CryptoKey, wrappingKey: CryptoKey, wrapAlgorithm: "AES-KW" | "RSA-OAEP" | Algorithm | EncryptAlgorithm): Promise<ArrayBuffer>;
    unwrapKey(format: KeyFormat, wrappedKey: BufferSource, unwrappingKey: CryptoKey, unwrapAlgorithm: "AES-KW" | "RSA-OAEP" | Algorithm | EncryptAlgorithm, unwrappedKeyAlgorithm: ImportAlgorithm, extractable: boolean, usages: KeyUsage[]): Promise<CryptoKey>;
  }

  class Crypto {
    constructor();
    readonly subtle: SubtleCrypto;
    getRandomValues<T extends ArrayBufferView>(array: T): T;
    randomUUID(): string;
  }

  export { Crypto, SubtleCrypto, CryptoKey };
  export default Crypto;
}
