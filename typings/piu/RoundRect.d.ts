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

// modules/piu/MC/outline/piuRoundRect.js (and the same content on Pebble)

declare module "piu/RoundRect" {
  import { Content, ContentDictionary } from "piu/MC";

  interface RoundRect extends Content {
    corners: number;
    radius: number;
  }
  interface RoundRectDictionary extends ContentDictionary {
    corners?: number;
    radius?: number;
  }
  interface RoundRectConstructor {
    new(behaviorData?: any, dictionary?: RoundRectDictionary): RoundRect;
    (behaviorData?: any, dictionary?: RoundRectDictionary): RoundRect;

    template<T>(this: T, fn: (arg: any) => RoundRectDictionary): T;

    readonly topLeft: 1;
    readonly topRight: 2;
    readonly bottomLeft: 4;
    readonly bottomRight: 8;
  }
  const RoundRect: RoundRectConstructor;

  global {
    const RoundRect: RoundRectConstructor
  }

  export { RoundRect };
  export default RoundRect;
}
