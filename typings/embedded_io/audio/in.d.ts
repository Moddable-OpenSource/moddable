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
declare module "embedded:io/audio/in" {
  type AudioType = "LPCM";

  interface AudioInOptions {
    bitsPerSample?: 8 | 16;
    channels?: 1 | 2;
    sampleRate?: number;
    audioType?: AudioType;
    format?: "buffer";
    target?: any;
  }

  class AudioIn {
    constructor(options: AudioInOptions & {
      onReadable?: (this: AudioIn, byteLength: number, sampleCount?: number) => void;
    });
    read(byteLength?: number): ArrayBuffer
    read(buffer: ByteBuffer): number
    start(): void;
    stop(options?: { flush?: boolean }): void
    close(): void;
    readonly bitsPerSample: 8 | 16;
    readonly channels: 1 | 2;
    readonly sampleRate: number;
    readonly audioType: AudioType;
    get format(): "buffer"
    set format(value: "buffer")
  }

  export { AudioIn as default };
}
