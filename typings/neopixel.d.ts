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

declare module "neopixel" {
    class NeoPixel {
        constructor(options?: {
            pin?: number,
            length?: number,
            order?: string
        });

        close(): void;
        update(): void;

        makeRGB(r: number, g: number, b: number): number;
        makeHSB(h: number, s: number, b: number): number;
        setPixel(index: number, color: number): void;
        fill(color: number, index?: number, count?: number): void;
        getPixel(index: number): number;

        get brightness(): number;
        set brightness(value: number);
        get length(): number;
        get byteLength(): number;
    }
    export default NeoPixel;
}
