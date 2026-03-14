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

/*
	SVGImage
*/

interface SVGImageDictionary extends ContentDictionary {
	clip?: boolean;
	id?: number;
	path?: string;
}

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

interface SVGImageConsructor {
	new(behaviorData?: any, dictionary?: SVGImageDictionary): SVGImage;
	template<T>(this: T, fn: (arg: any) => SVGImageDictionary): T;
  (behaviorData?: any, dictionary?: SVGImageDictionary): SVGImage;
}

/*
	RoundRect
*/

interface RoundRectDictionary extends ContentDictionary {
	corners?: number;
	radius?: number;
}

interface RoundRect extends Content {
	corners: number;
	radius: number;
}

interface RoundRectConstructor {
	new(behaviorData?: any, dictionary?: RoundRectDictionary): RoundRect;
	template<T>(this: T, fn: (arg: any) => RoundRectDictionary): T;
	(behaviorData?: any, dictionary?: RoundRectDictionary): RoundRect;

	readonly topLeft: 1;
	readonly topRight: 2;
	readonly bottomLeft: 4;
	readonly bottomRight: 8;
}

/*
	Inverter
*/

interface Inverter extends Content {
}

interface InverterConstructor {
	new(behaviorData?: any, dictionary?: ContentDictionary): Inverter;
	template<T>(this: T, fn: (arg: any) => ContentDictionary): T;
}

/*
	ScreenBuffer
*/

interface ScreenBuffer extends Content {
}

interface ScreenBufferConstructor {
	new(behaviorData?: any, dictionary?: ContentDictionary): ScreenBuffer;
	template<T>(this: T, fn: (arg: any) => ContentDictionary): T;
}
