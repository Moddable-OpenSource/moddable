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

// on Pebble, a texture may be created from the id of a GBitmap resource (modules/piu/Pebble/piuPebble.js).
// A module, so that this augments the real module piu/MC-types rather than replacing it.

import type { Texture } from "piu/MC-types";
import type Bitmap from "commodetto/Bitmap";

declare module "piu/MC-types" {
	interface TextureConstructor {
		new(resourceID: number): Texture;
		/** a texture over bitmaps already in memory, for example from parseRLE */
		new(path: null | undefined, alphaBitmap: Bitmap | null | undefined, colorBitmap?: Bitmap | null): Texture;
	}
}
