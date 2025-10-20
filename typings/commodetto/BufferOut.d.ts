/*
* Copyright (c) 2025 Moddable Tech, Inc.
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

declare module "commodetto/BufferOut" {
  import type Bitmap from "commodetto/Bitmap";
  import type {Rectangle} from "commodetto/Poco";

  interface BufferOutOptions {
    width: number;
    height: number;
    pixelFormat: number;
    buffer?: BufferLike;
  }

  class BufferOut {
    constructor(options: BufferOutOptions);
    begin(x: number, y: number, width: number, height: number): void;
    end(): void;
    continue(x: number, y: number, width: number, height: number): void;
    send(
      pixels: BufferLike,
      offset: number,
      count: number
    ): void;

    pixelsToBytes(count: number): number;
    adaptInvalid(area: Rectangle): void;

    readonly width: number;
    readonly height: number;
    readonly pixelFormat: number;
    readonly bitmap: Bitmap;
  }

  export {BufferOut as default};
}
