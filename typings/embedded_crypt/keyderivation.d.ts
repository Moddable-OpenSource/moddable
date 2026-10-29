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

declare module "embedded:crypt/keyderivation" {
  import { MACHash } from "embedded:crypt/MAC";
  export type KeyDerivationAlgorithm = "HKDF" | "HKDF-Extract" | "HKDF-Expand" | "PBKDF2";

  interface KeyDerivationOptionsBase {
    algorithm: KeyDerivationAlgorithm;
    hash: MACHash;
    key: ByteBuffer;
  }
  interface HKDFOptions extends KeyDerivationOptionsBase {
    algorithm: "HKDF";
    salt?: ByteBuffer;
    info?: ByteBuffer;
  }
  interface HKDFExtractOptions extends KeyDerivationOptionsBase {
    algorithm: "HKDF-Extract";
    salt?: ByteBuffer;
  }
  interface HKDFExpandOptions extends KeyDerivationOptionsBase {
    algorithm: "HKDF-Expand";
    info?: ByteBuffer;
  }
  interface PBKDF2Options extends KeyDerivationOptionsBase {
    algorithm: "PBKDF2";
    salt?: ByteBuffer;
    iterations: number;
  }
  type KeyDerivationOptions = HKDFOptions | HKDFExtractOptions | HKDFExpandOptions | PBKDF2Options;

  class KeyDerivation implements Disposable {
    constructor(options: KeyDerivationOptions);
    static supports(options: Partial<KeyDerivationOptions>): boolean;
    read(count: number): ArrayBuffer | undefined;
    read(buffer: ByteBuffer): number | undefined;
    close(): void;
    readonly algorithm: KeyDerivationAlgorithm;
    readonly hash: MACHash;
    [Symbol.dispose](): void;
  }

  export default KeyDerivation;
}
