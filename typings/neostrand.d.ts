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

declare module "neostrand" {
    import type NeoPixel from "neopixel";

    type NeoStrandEffectDictionary = {
        strand: NeoStrand;
        start?: number;
        end?: number;
        reverse?: 0 | 1;
        duration?: number;
        position?: number;
        speed?: number;
        size?: number;
        loop?: 0 | 1;
        tickle?: number;
        onComplete?: (effect: NeoStrandEffect) => void;
    }

    class NeoStrandEffect {
        constructor(dictionary: NeoStrandEffectDictionary);

        reset(effect: NeoStrandEffect): void;
        loopPrepare(effect: NeoStrandEffect): void;
        activate(effect: NeoStrandEffect): void;
        idle(effect: NeoStrandEffect, ticks: number): void;
    }

    class HueSpan extends NeoStrandEffect {
        constructor(dictionary: NeoStrandEffectDictionary & {
            position?: number;
            saturation?: number;
            value?: number;
        });

        set hue(h: number);
    }

    class Sine extends NeoStrandEffect {
        constructor(dictionary: NeoStrandEffectDictionary & {
            amplitude?: number;
            offset?: number;
            vary?: "r" | "g" | "b" | "h" | "s" | "v";
        });

        set effectValue(value: number);
    }

    interface rgb {
        r: number;
        g: number;
        b: number;
    }
    class Marquee extends NeoStrandEffect {
        constructor(dictionary: NeoStrandEffectDictionary & {
            sizeA?: number;
            sizeB?: number;
            rgbA?: rgb;
            rgbB?: rgb;
        });

        set steps(s: number);
    }

    class Pulse extends NeoStrandEffect {
        constructor(dictionary: NeoStrandEffectDictionary & {
            dir?: number;
            mode?: -1 | 0 | 1;
            fade?: number;
            size?: number;
            duration?: number;
            position?: "random" | number;
            rgb?: rgb;
        });

        loopPrepare(effect: NeoStrandEffect): void;
        set pulseLoc(px: number);
    }

    class Pattern extends NeoStrandEffect {
        constructor(dictionary: NeoStrandEffectDictionary & {
            pattern?: number[];
            mode?: -1 | 0 | 1;
        });

        loopPrepare(effect: NeoStrandEffect): void;
        set effectValue(value: number);
    }

    class Dim extends NeoStrandEffect {
        constructor(dictionary: NeoStrandEffectDictionary & {
            rgb?: rgb;
        });
    }

    class Ease extends NeoStrandEffect {
        constructor(dictionary: NeoStrandEffectDictionary & {
            easing?: number;
            size?: number;
            rgb?: rgb;
        });

        set steps(s: number);
    }

    interface PulseTiming {
        level0: 0 | 1;
        duration0: number;
        level1: 0 | 1;
        duration1: number;
    }

    class NeoStrand extends NeoPixel {
        constructor(options?: {
            pin?: number,
            length?: number,
            order?: string,
            timeings?: {
                mark: PulseTiming,
                space: PulseTiming,
                reset: PulseTiming
            }
        });

        set(idx: number, color: number, start?: number, end?: number): void;
        add(idx: number, color: number, start?: number, end?: number): void;
        sub(idx: number, color: number, start?: number, end?: number): void;
        op(idx: number, rgb: number, mode: number, start: number, end: number): void;
        setScheme(scheme: NeoStrandEffect[]): void;
        start(ms?: number): void;
        stop(): void;
        hsvToRgb(h: number, s: number, v: number): number;
        rgbToHsv(r: number, g: number, b: number): { h: number; s: number; v: number };

        static HueSpan: HueSpan;
        static Sine: Sine;
        static Marquee: Marquee;
        static Pulse: Pulse;
        static Pattern: Pattern;
        static Dim: Dim;
        static Ease: Ease;
    }
}
