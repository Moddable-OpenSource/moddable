/*
* Copyright (c) 2022 Satoshi Tanaka
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

declare module "text/decoder" {
  interface TextDecoderOptions {
    fatal?: boolean;
    ignoreBOM?: boolean;
  }
  interface TextDecodeOptions {
    stream?: boolean;
  }

  interface TextDecoder extends TextDecoderCommon {
    decode(input?: Uint8Array | ArrayBufferLike, options?: TextDecodeOptions): string;
  }

  var TextDecoder: {
    prototype: TextDecoder;
    new(label?: "utf-8", options?: TextDecoderOptions): TextDecoder;
  };

  interface TextDecoderCommon {
    readonly encoding: string;
    readonly fatal: boolean;
    readonly ignoreBOM: boolean;
  }

  export { TextDecoder as default };
}
