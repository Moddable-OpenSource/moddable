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

/// <reference path="../commodetto/Poco.d.ts" />


declare module "commodetto/Poco" {
  import Bitmap from "commodetto/Bitmap";

  interface FontConstructor {
		new(family: string, size: number): Font;
	}

	interface PocoPrototype {
		Font: FontConstructor;
		unobstructed: {
			readonly x: number;
			readonly y: number;
			readonly width: number;
			readonly height: number
		};
		drawLine(x0: number, y0: number, x1: number, y1: number, color: number, width: number): void;
		drawRoundRect(x0: number, y0: number, x1: number, y1: number, color: number, radius: number, corners: number): void;
		frameRoundRect(x0: number, y0: number, x1: number, y1: number, color: number, radius: number): void;
		drawCircle(color: number, x: number, y: number, r: number, from: number, to: number): void;
		drawDCI(dci: PebbleDrawCommandImage | PebbleDrawCommandSequence, x: number, y: number): void;
	}

  class PebbleBitmap extends Bitmap {
    constructor(id: number | string);
  }

  class PebbleDrawCommandList {
    scale(x: number, y: number): this;
    scale(scale: number): this;
    rotate(angle: number, cx: number, cy: number): this;
  }

  class PebbleDrawCommandImage extends PebbleDrawCommandList {
    constructor(id: number | string);
    readonly width: number;
    readonly height: number;
    clone(): PebbleDrawCommandImage;
  }

  class PebbleDrawCommandSequence {
    constructor(id: number | string);
    readonly width: number;
    readonly height: number;
    readonly duration: number;
    readonly frameDuration: number;
    time: number;
    clone(): PebbleDrawCommandSequence;
  }

  interface PocoConstructor {
    PebbleBitmap: typeof PebbleBitmap;
    PebbleDrawCommandImage: typeof PebbleDrawCommandImage;
    PebbleDrawCommandSequence: typeof PebbleDrawCommandSequence;
    PebbleDrawCommandList: typeof PebbleDrawCommandList;
  }
}
