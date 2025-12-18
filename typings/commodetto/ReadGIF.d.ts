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

declare module "commodetto/ReadGIF" {
  import Bitmap from "commodetto/Bitmap"

  class ReadGIF extends Bitmap {
    constructor(buffer: BufferLike);
    close(): void;
    first(): void;
    next(): void;

    readonly duration: number
    readonly frameBounds: {x: number, y: number, width: number, height: number}
    readonly frameCount: number
    readonly frameDuration: number
    readonly frameNumber: number
    readonly frameX: number
    readonly frameY: number
    readonly frameWidth: number
    readonly frameHeight: number

    readonly transparent: boolean;
    transparentColor: number;

    readonly ready: boolean
    avaiable: number
  }

  export {ReadGIF as default};
}

declare module "commodetto/Poco" {
  import ReadGIF from "commodetto/ReadGIF"

  interface PocoPrototype {
    drawBitmapWithKeyColor(reader: ReadGIF, x: number, y: number): void
  }
}
