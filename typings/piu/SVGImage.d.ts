/*
 * Copyright (c) 2026 Moddable Tech, Inc
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

// modules/piu/MC/outline/piuSVGImage.js (and the same content on Pebble)

declare module "piu/SVGImage" {
  import { Content, ContentDictionary } from "piu/MC";

  interface SVGImage extends Content {
    opacity: number;
    center(x: number, y: number): void;
    cx: number;
    cy: number;
    rotate(a: number): void;
    r: number;
    scale(x: number, y: number): void;
    s: number;
    sx: number;
    sy: number;
    translate(x: number, y: number): void;
    tx: number;
    ty: number;
  }
  interface SVGImageDictionary extends ContentDictionary {
    clip?: boolean;
    /** the path of the SVG resource */
    path?: string;
    /** the SVG resource itself, as parsed by load */
    resource?: object;
  }
  interface SVGImageConstructor {
    new(behaviorData?: any, dictionary?: SVGImageDictionary): SVGImage;
    (behaviorData?: any, dictionary?: SVGImageDictionary): SVGImage;

    template<T>(this: T, fn: (arg: any) => SVGImageDictionary): T;
  }
  const SVGImage: SVGImageConstructor;

  global {
    const SVGImage: SVGImageConstructor
  }

  export { SVGImage };
  export default SVGImage;
}
