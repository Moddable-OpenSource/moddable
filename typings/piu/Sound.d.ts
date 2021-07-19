/*
 * Copyright (c) 2020 Shinya Ishikawa
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
declare module "piu/Sound" {
  import AudioOut from "pins/audioout"
  class Sound {
    public constructor(dictionary: {
      path: string;
      offset?: number;
      size?: number;
    });
    public constructor(tones: {
      frequency: number,
      samples?: number
    }[]);
    public static callback(index: number): void;
    public static readonly bitsPerSample: number;
    public static readonly numChannels: number;
    public static readonly sampleRate: number;
    public static get volume(): number;
    public static set volume(it: number);
    public static close(): void;
    public static open(stream?: number): AudioOut;
    public play(stream?: number, repeat?: number, callback?: Function): void;
  }
  export { Sound as default }
}
